/// @file      grpc_service_host.hpp
/// @brief     gRPC Async + CQ 服务端封装（纯传输层，与支撑服务接口无关）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_GRPC_SERVICE_HOST_HPP_
#define QTRADE_COMMON_GRPC_GRPC_SERVICE_HOST_HPP_

#include "common/grpc/grpc_async_server.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <string_view>

namespace qtrade::common::grpc_async {

/// @brief gRPC Async + CQ 服务端封装
/// @tparam AsyncServiceT protobuf 生成的 gRPC AsyncService 类型
/// @tparam HandlerT RPC 异步处理器，需提供 Init / Start / Shutdown
/// @tparam RepositoryT 注入 Handler 的仓储类型
template <typename AsyncServiceT, typename HandlerT, typename RepositoryT>
class GrpcServiceHost {
 public:
  GrpcServiceHost() = default;

  /// @brief 析构并调用 Shutdown
  ~GrpcServiceHost() {
    Shutdown();
  }

  GrpcServiceHost(const GrpcServiceHost&) = delete;
  GrpcServiceHost& operator=(const GrpcServiceHost&) = delete;

  /// @brief 启动 gRPC 监听
  /// @param listen_address 监听地址，如 0.0.0.0:50051
  /// @param repository 数据库仓储
  /// @param log_tag 日志标识
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Start(const std::string& listen_address,
                  const std::shared_ptr<RepositoryT>& repository,
                  const std::string_view log_tag) {
    if (running_) {
      return ErrorCode::kSystemError;
    }
    if (!repository) {
      return ErrorCode::kInternal;
    }

    repository_ = repository;
    grpc_server_ = std::make_unique<GrpcAsyncServer>();
    handler_ = std::make_unique<HandlerT>();

    GrpcAsyncServer::Options opts;
    opts.listen_address = listen_address;
    opts.cq_thread_count = 1;

    if (const auto rc = grpc_server_->Start(opts, &async_service_); rc != ErrorCode::kSuccess) {
      handler_.reset();
      grpc_server_.reset();
      repository_.reset();
      return rc;
    }

    handler_->Init(&async_service_, grpc_server_->CompletionQueue(), repository_);
    handler_->Start();

    running_ = true;
    spdlog::info("[{}] listening on {} (async+cq)", log_tag, listen_address);
    return ErrorCode::kSuccess;
  }

  /// @brief 优雅停止 gRPC 服务（不阻塞 Wait）
  void Shutdown() {
    if (!running_) {
      return;
    }

    if (handler_) {
      handler_->Shutdown();
      handler_.reset();
    }
    if (grpc_server_) {
      grpc_server_->Shutdown();
    }

    repository_.reset();
    running_ = false;
  }

  /// @brief 阻塞直至 gRPC 服务端退出
  void Wait() {
    if (grpc_server_) {
      grpc_server_->Wait();
      grpc_server_.reset();
    }
  }

  /// @brief gRPC 是否正在运行
  [[nodiscard]] bool IsRunning() const {
    return running_;
  }

 private:
  AsyncServiceT async_service_;                   ///< protobuf 异步服务实例
  std::unique_ptr<GrpcAsyncServer> grpc_server_;  ///< gRPC 服务端
  std::unique_ptr<HandlerT> handler_;             ///< RPC 异步处理器
  std::shared_ptr<RepositoryT> repository_;       ///< 数据库仓储
  bool running_ = false;                          ///< 是否已启动
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_GRPC_SERVICE_HOST_HPP_
