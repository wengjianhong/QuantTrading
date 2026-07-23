/// @file      support_async_service_impl.hpp
/// @brief     gRPC 异步支撑服务通用基类（AsyncService + CQ）
/// @details   直接持有 GrpcAsyncServer、AsyncService 与 Handler；适用于含 Streaming 的支撑服务
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SUPPORT_SUPPORT_ASYNC_SERVICE_IMPL_HPP_
#define QTRADE_COMMON_SUPPORT_SUPPORT_ASYNC_SERVICE_IMPL_HPP_

#include "qtrade/framework/database/db_connection_pool_manager.hpp"
#include "qtrade/framework/grpc/async/grpc_async_server.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/support/support_service.hpp>

#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace qtrade::common::support {

/// @brief gRPC 异步支撑服务基类（AsyncService + CQ）
/// @tparam AsyncServiceT protobuf 生成的 gRPC AsyncService 类型
/// @tparam HandlerT RPC 异步处理器，需提供 Init / Start / Shutdown
template <typename AsyncServiceT, typename HandlerT>
class SupportAsyncServiceImpl : public ISupportService {
 public:
  /// @brief 构造异步支撑服务基类
  /// @param service_name 服务名（日志与状态快照）
  /// @param default_port 未配置端口时的默认监听端口
  SupportAsyncServiceImpl(std::string service_name, int default_port)
    : service_name_(std::move(service_name)), default_port_(default_port) {}

  /// @brief 析构时调用 Stop 释放 Handler、Server 与连接资源
  ~SupportAsyncServiceImpl() override {
    Stop();
  }

  SupportAsyncServiceImpl(const SupportAsyncServiceImpl&) = delete;
  SupportAsyncServiceImpl& operator=(const SupportAsyncServiceImpl&) = delete;

  /// @brief 读取配置并初始化依赖（由子类实现）
  /// @param config_path 配置文件路径
  /// @return 成功返回 ErrorCode::kSuccess；失败返回对应错误码
  ErrorCode Initialize(const std::string& config_path) override = 0;

  /// @brief 启动 Async Server、注入 Handler 并开始接受请求
  /// @return 成功返回 ErrorCode::kSuccess；状态非法、DB 未就绪或监听失败返回错误码
  /// @details 要求状态为 kInitializing 且 connection_pool_mgr_ 已就绪；失败时写入 last_error_ 并置为 kFailed
  ErrorCode Start() override {
    std::lock_guard lock(mutex_);

    // 1. 校验生命周期状态与数据库连接
    if (state_ != SupportServiceState::kInitializing || !connection_pool_mgr_ || !connection_pool_mgr_->IsReady()) {
      return ErrorCode::kSystemError;
    }

    if (grpc_server_ && grpc_server_->IsRunning()) {
      return ErrorCode::kSystemError;
    }

    // 2. 创建 Async Server 与 Handler，并启动监听
    grpc_server_ = std::make_unique<grpc_async::GrpcAsyncServer>();
    handler_ = std::make_unique<HandlerT>();

    grpc_async::GrpcAsyncServer::Options opts;
    opts.listen_address = listen_address_;
    opts.cq_thread_count = 1;

    if (const auto rc = grpc_server_->Start(opts, &async_service_); rc != ErrorCode::kSuccess) {
      handler_.reset();
      grpc_server_.reset();
      state_ = SupportServiceState::kFailed;
      last_error_ = rc;
      return rc;
    }

    // 3. 绑定 CQ / 连接（及子类额外依赖）后启动 Handler，并标记就绪
    InitHandler();
    handler_->Start();

    state_ = SupportServiceState::kReady;
    last_error_ = ErrorCode::kSuccess;
    spdlog::info("[{}] listening on {} (async+cq)", service_name_, listen_address_);
    return ErrorCode::kSuccess;
  }

  /// @brief 优雅停止 Handler 与 gRPC 服务（不阻塞 Wait）
  /// @details Shutdown 后保留 grpc_server_ 供 Wait 阻塞；若尚未 Start 则仅清理初始化阶段资源
  void Stop() override {
    std::lock_guard lock(mutex_);
    if (!grpc_server_ || !grpc_server_->IsRunning()) {
      if (state_ == SupportServiceState::kInitializing) {
        connection_pool_mgr_.reset();
        state_ = SupportServiceState::kTerminated;
      }
      return;
    }

    state_ = SupportServiceState::kStopping;
    if (handler_) {
      handler_->Shutdown();
      handler_.reset();
    }
    grpc_server_->Shutdown();
    connection_pool_mgr_.reset();
    state_ = SupportServiceState::kTerminated;
  }

  /// @brief 阻塞直至 gRPC Server 完全退出
  void Wait() override {
    if (grpc_server_) {
      grpc_server_->Wait();
      grpc_server_.reset();
    }
  }

  /// @brief 获取当前运行状态快照
  /// @return 含服务名、配置路径、监听地址、错误码与生命周期状态
  [[nodiscard]] SupportServiceStatus GetStatus() const override {
    std::lock_guard lock(mutex_);
    return SupportServiceStatus{
      .service_name = service_name_,
      .config_path = config_path_,
      .listen_address = listen_address_,
      .last_error_message = {},
      .last_error = last_error_,
      .state = state_,
    };
  }

 protected:
  /// @brief 向 Handler 注入 AsyncService / CQ / 连接等依赖（子类可追加 DaoManager）
  virtual void InitHandler() {
    handler_->Init(&async_service_, grpc_server_->CompletionQueue(), connection_pool_mgr_);
  }

  /// 配置文件路径
  std::string config_path_;
  /// 服务名
  std::string service_name_;
  /// gRPC 监听地址（host:port）
  std::string listen_address_;

  /// 默认监听端口
  int default_port_ = -1;
  /// 最近一次错误码
  ErrorCode last_error_ = ErrorCode::kSuccess;
  /// 生命周期状态
  SupportServiceState state_ = SupportServiceState::kNew;

  /// 保护启停与状态字段
  mutable std::mutex mutex_;
  /// protobuf AsyncService 实例
  AsyncServiceT async_service_;
  /// 异步 RPC Handler
  std::unique_ptr<HandlerT> handler_;
  /// 异步 gRPC Server（含 CQ）
  std::unique_ptr<grpc_async::GrpcAsyncServer> grpc_server_;
  /// 数据库连接池管理器
  std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection_pool_mgr_;
};

}  // namespace qtrade::common::support

#endif  // QTRADE_COMMON_SUPPORT_SUPPORT_ASYNC_SERVICE_IMPL_HPP_
