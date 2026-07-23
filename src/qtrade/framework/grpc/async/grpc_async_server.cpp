/// @file      grpc_async_server.cpp
/// @brief     GrpcAsyncServer 实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/grpc/async/grpc_async_server.hpp"

#include "qtrade/framework/grpc/async/completion_queue_loop.hpp"

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
  // 1. 校验运行状态与入参
  if (running_) {
    return ErrorCode::kSystemError;
  }
  if (options.listen_address.empty() || async_service == nullptr) {
    return ErrorCode::kInternalError;
  }

  // 2. 构建 Server、注册 Service 并创建 CQ
  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(options.listen_address, grpc::InsecureServerCredentials());
  builder.RegisterService(async_service);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  if (!server_ || !cq_) {
    cq_.reset();
    server_.reset();
    return ErrorCode::kInternalError;
  }

  // 3. 启动 CQ 轮询线程
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

  // 1. 先关 Server 与 CQ，再停轮询线程
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
