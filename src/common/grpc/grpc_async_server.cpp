/// @file      grpc_async_server.cpp
/// @brief     GrpcAsyncServer 实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "common/grpc/grpc_async_server.hpp"

#include "common/grpc/completion_queue_loop.hpp"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/resource_quota.h>
#include <spdlog/spdlog.h>

namespace qtrade::common::grpc_async {

GrpcAsyncServer::GrpcAsyncServer() = default;

GrpcAsyncServer::~GrpcAsyncServer() {
  Shutdown();
  Wait();
}

ErrorCode GrpcAsyncServer::Start(const Options& options, grpc::Service* async_service) {
  if (running_) {
    return ErrorCode::kSystemError;
  }
  if (options.listen_address.empty() || async_service == nullptr) {
    return ErrorCode::kInternal;
  }

  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(options.listen_address, grpc::InsecureServerCredentials());
  builder.RegisterService(async_service);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  if (!server_ || !cq_) {
    cq_.reset();
    server_.reset();
    return ErrorCode::kInternal;
  }

  loop_ = std::make_unique<CompletionQueueLoop>();
  loop_->Start(cq_.get(), options.cq_thread_count);

  running_ = true;
  spdlog::info(
    "[GrpcAsyncServer] listening on {} (async, cq_threads={})", options.listen_address, options.cq_thread_count);
  return ErrorCode::kSuccess;
}

void GrpcAsyncServer::Shutdown() {
  if (!running_) {
    return;
  }

  if (server_) {
    server_->Shutdown();
  }
  if (cq_) {
    cq_->Shutdown();
  }
  if (loop_) {
    loop_->Stop();
    loop_.reset();
  }
  running_ = false;
  spdlog::info("[GrpcAsyncServer] stopped");
}

void GrpcAsyncServer::Wait() {
  if (server_) {
    server_->Wait();
    server_.reset();
  }
  cq_.reset();
}

}  // namespace qtrade::common::grpc_async
