/// @file      grpc_sync_service_host.hpp
/// @brief     gRPC 同步服务端封装（纯传输层）
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_GRPC_SYNC_SERVICE_HOST_HPP_
#define QTRADE_COMMON_GRPC_GRPC_SYNC_SERVICE_HOST_HPP_

#include "qtrade/framework/grpc/grpc_sync_server.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <string_view>

namespace grpc {
class Service;
}

namespace qtrade::common::grpc_sync {

/// @brief gRPC 同步服务端封装
class GrpcSyncServiceHost {
 public:
  GrpcSyncServiceHost() = default;

  ~GrpcSyncServiceHost() {
    Shutdown();
  }

  GrpcSyncServiceHost(const GrpcSyncServiceHost&) = delete;
  GrpcSyncServiceHost& operator=(const GrpcSyncServiceHost&) = delete;

  /// @brief 启动 gRPC 监听
  ErrorCode Start(const std::string& listen_address, grpc::Service* sync_service, const std::string_view log_tag) {
    if (running_) {
      return ErrorCode::kSystemError;
    }
    if (sync_service == nullptr) {
      return ErrorCode::kInternal;
    }

    sync_service_ = sync_service;
    grpc_server_ = std::make_unique<GrpcSyncServer>();
    if (const auto rc = grpc_server_->Start(listen_address, sync_service_); rc != ErrorCode::kSuccess) {
      grpc_server_.reset();
      sync_service_ = nullptr;
      return rc;
    }

    running_ = true;
    spdlog::info("[{}] listening on {} (sync)", log_tag, listen_address);
    return ErrorCode::kSuccess;
  }

  void Shutdown() {
    if (!running_) {
      return;
    }

    if (grpc_server_) {
      grpc_server_->Shutdown();
    }

    sync_service_ = nullptr;
    running_ = false;
  }

  void Wait() {
    if (grpc_server_) {
      grpc_server_->Wait();
      grpc_server_.reset();
    }
  }

  [[nodiscard]] bool IsRunning() const {
    return running_;
  }

 private:
  grpc::Service* sync_service_ = nullptr;
  std::unique_ptr<GrpcSyncServer> grpc_server_;
  bool running_ = false;
};

}  // namespace qtrade::common::grpc_sync

#endif  // QTRADE_COMMON_GRPC_GRPC_SYNC_SERVICE_HOST_HPP_
