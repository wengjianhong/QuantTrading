/// @file      support_sync_service_impl.hpp
/// @brief     gRPC 同步支撑服务通用基类（Unary RPC）
/// @details   直接持有 GrpcSyncServer 与同步 Service 实现；子类负责 Initialize 注入配置与连接
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SUPPORT_SUPPORT_SYNC_SERVICE_IMPL_HPP_
#define QTRADE_COMMON_SUPPORT_SUPPORT_SYNC_SERVICE_IMPL_HPP_

#include "qtrade/framework/database/db_connection_pool_manager.hpp"
#include "qtrade/framework/grpc/sync/grpc_sync_server.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/support/support_service.hpp>

#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace qtrade::common::support {

/// @brief gRPC 同步支撑服务基类
/// @tparam GrpcServiceT 继承自 protobuf 生成的 xxx::Service 的实现类
template <typename GrpcServiceT>
class SupportSyncServiceImpl : public ISupportService {
 public:
  /// @brief 构造同步支撑服务基类
  /// @param service_name 服务名（日志与状态快照）
  /// @param default_port 未配置端口时的默认监听端口
  SupportSyncServiceImpl(std::string service_name, int default_port)
    : service_name_(std::move(service_name)), default_port_(default_port) {}

  /// @brief 析构时调用 Stop 释放 gRPC 与连接资源
  ~SupportSyncServiceImpl() override {
    Stop();
  }

  SupportSyncServiceImpl(const SupportSyncServiceImpl&) = delete;
  SupportSyncServiceImpl& operator=(const SupportSyncServiceImpl&) = delete;

  /// @brief 读取配置并初始化依赖（由子类实现）
  /// @param config_path 配置文件路径
  /// @return 成功返回 ErrorCode::kSuccess；失败返回对应错误码
  ErrorCode Initialize(const std::string& config_path) override = 0;

  /// @brief 创建同步 Service 并启动 gRPC 监听
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

    // 2. 创建 Service 并启动同步 Server
    grpc_service_ = CreateGrpcService();
    if (!grpc_service_) {
      state_ = SupportServiceState::kFailed;
      last_error_ = ErrorCode::kSystemError;
      return last_error_;
    }
    grpc_server_ = std::make_unique<grpc_sync::GrpcSyncServer>();
    if (const auto rc = grpc_server_->Start(listen_address_, grpc_service_.get()); rc != ErrorCode::kSuccess) {
      grpc_server_.reset();
      grpc_service_.reset();
      state_ = SupportServiceState::kFailed;
      last_error_ = rc;
      return rc;
    }

    // 3. 标记就绪并记录监听地址
    state_ = SupportServiceState::kReady;
    last_error_ = ErrorCode::kSuccess;
    spdlog::info("[{}] listening on {} (sync)", service_name_, listen_address_);
    return ErrorCode::kSuccess;
  }

  /// @brief 优雅停止 gRPC 服务并释放连接（不阻塞 Wait）
  /// @details Shutdown 后保留 grpc_server_ 供 Wait 阻塞；若尚未 Start 则仅清理初始化阶段资源
  void Stop() override {
    std::lock_guard lock(mutex_);
    if (!grpc_server_ || !grpc_server_->IsRunning()) {
      if (state_ == SupportServiceState::kInitializing) {
        grpc_service_.reset();
        connection_pool_mgr_.reset();
        state_ = SupportServiceState::kTerminated;
      }
      return;
    }

    state_ = SupportServiceState::kStopping;
    grpc_server_->Shutdown();
    grpc_service_.reset();
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
  /// @brief 创建同步 gRPC Service（子类注入 connection_pool_mgr_ / DaoManager 等依赖）
  /// @return 非空表示创建成功
  [[nodiscard]] virtual std::unique_ptr<GrpcServiceT> CreateGrpcService() = 0;

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
  /// 同步 gRPC Service 实现
  std::unique_ptr<GrpcServiceT> grpc_service_;
  /// 同步 gRPC Server
  std::unique_ptr<grpc_sync::GrpcSyncServer> grpc_server_;
  /// 数据库连接池管理器
  std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection_pool_mgr_;
};

}  // namespace qtrade::common::support

#endif  // QTRADE_COMMON_SUPPORT_SUPPORT_SYNC_SERVICE_IMPL_HPP_
