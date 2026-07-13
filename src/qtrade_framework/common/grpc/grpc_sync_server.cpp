/// @file      grpc_sync_server.cpp
/// @brief     GrpcSyncServer 实现
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade_framework/common/grpc/grpc_sync_server.hpp"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <spdlog/spdlog.h>

namespace qtrade::common::grpc_sync {

GrpcSyncServer::GrpcSyncServer() = default;

GrpcSyncServer::~GrpcSyncServer() {
  Shutdown();
  Wait();
}

ErrorCode GrpcSyncServer::Start(const std::string& listen_address, grpc::Service* sync_service) {
  if (running_) {
    return ErrorCode::kSystemError;
  }
  if (listen_address.empty() || sync_service == nullptr) {
    return ErrorCode::kInternal;
  }

  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(listen_address, grpc::InsecureServerCredentials());
  builder.RegisterService(sync_service);
  server_ = builder.BuildAndStart();
  if (!server_) {
    return ErrorCode::kInternal;
  }

  running_ = true;
  spdlog::info("[GrpcSyncServer] listening on {} (sync)", listen_address);
  return ErrorCode::kSuccess;
}

void GrpcSyncServer::Shutdown() {
  if (!running_) {
    return;
  }

  if (server_) {
    server_->Shutdown();
  }
  running_ = false;
  spdlog::info("[GrpcSyncServer] stopped");
}

void GrpcSyncServer::Wait() {
  if (server_) {
    server_->Wait();
    server_.reset();
  }
}

}  // namespace qtrade::common::grpc_sync
