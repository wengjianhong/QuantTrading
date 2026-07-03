/// @file      config_server.cpp
/// @brief     ConfigServer 实现
/// @author    wengjianhong
/// @date      2026-06-22
/// @copyright CC BY-NC-SA 4.0
#include "service/config_service/config_server.hpp"

#include "common/database/database_options.hpp"
#include "common/grpc/grpc_async_server.hpp"
#include "service/config_service/config_grpc_async_handler.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::service {

ConfigServer::ConfigServer() = default;

ConfigServer::~ConfigServer() { Shutdown(); }

ErrorCode ConfigServer::Start(const std::string& listen_address, const ConfigServiceContext& context) {
  if (running_) {
    return ErrorCode::kSystemError;
  }
  if (!context.repository) {
    return ErrorCode::kInternal;
  }

  repository_ = context.repository;
  grpc_server_ = std::make_unique<qtrade::common::grpc_async::GrpcAsyncServer>();
  handler_ = std::make_unique<ConfigGrpcAsyncHandler>();

  qtrade::common::grpc_async::GrpcAsyncServer::Options opts;
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
  spdlog::info("[ConfigServer] listening on {} (async+cq)", listen_address);
  return ErrorCode::kSuccess;
}

void ConfigServer::Shutdown() {
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

void ConfigServer::Wait() {
  if (grpc_server_) {
    grpc_server_->Wait();
    grpc_server_.reset();
  }
}

ConfigServiceContext BootstrapConfigService(const std::string& json_path) {
  ConfigServiceContext context;

  const auto database_options = qtrade::common::ParseDatabaseOptions(json_path);
  if (!database_options.enabled) {
    spdlog::error("[ConfigServer] database disabled");
    return context;
  }

  context.repository = CreateConfigRepository(database_options);
  if (!context.repository) {
    spdlog::error("[ConfigServer] create repository failed");
    return context;
  }

  if (const auto rc = context.repository->EnsureSchema(); rc != ErrorCode::kSuccess) {
    spdlog::error("[ConfigServer] ensure schema failed");
    context.repository.reset();
    return context;
  }

  spdlog::info("[ConfigServer] database ready");
  return context;
}

}  // namespace qtrade::service
