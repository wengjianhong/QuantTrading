/// @file      account_server.cpp
/// @brief     AccountServer 实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "service/account_service/account_server.hpp"

#include "common/database/database_options.hpp"
#include "common/grpc/grpc_async_server.hpp"
#include "service/account_service/account_grpc_async_handler.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::service {

AccountServer::AccountServer() = default;

AccountServer::~AccountServer() { Shutdown(); }

ErrorCode AccountServer::Start(const std::string& listen_address, const AccountServiceContext& context) {
  if (running_) {
    return ErrorCode::kSystemError;
  }
  if (!context.repository) {
    return ErrorCode::kInternal;
  }

  repository_ = context.repository;
  grpc_server_ = std::make_unique<qtrade::common::grpc_async::GrpcAsyncServer>();
  handler_ = std::make_unique<AccountGrpcAsyncHandler>();

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
  spdlog::info("[AccountServer] listening on {} (async+cq)", listen_address);
  return ErrorCode::kSuccess;
}

void AccountServer::Shutdown() {
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

void AccountServer::Wait() {
  if (grpc_server_) {
    grpc_server_->Wait();
    grpc_server_.reset();
  }
}

AccountServiceContext BootstrapAccountService(const std::string& json_path) {
  AccountServiceContext context;

  const auto database_options = qtrade::common::ParseDatabaseOptions(json_path);
  if (!database_options.enabled) {
    spdlog::error("[AccountServer] database disabled");
    return context;
  }

  context.repository = CreateAccountRepository(database_options);
  if (!context.repository) {
    spdlog::error("[AccountServer] create repository failed");
    return context;
  }

  if (const auto rc = context.repository->EnsureSchema(); rc != ErrorCode::kSuccess) {
    spdlog::error("[AccountServer] ensure schema failed");
    context.repository.reset();
    return context;
  }

  spdlog::info("[AccountServer] database ready");
  return context;
}

}  // namespace qtrade::service
