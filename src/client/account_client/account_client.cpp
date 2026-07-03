/// @file      account_client.cpp
/// @brief     交易账户凭证客户端实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "client/account_client/account_client.hpp"

#include <qtrade/account/v1/account.grpc.pb.h>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <chrono>

namespace qtrade::client {

struct AccountClient::Impl {
  AccountClientOptions options;
  std::shared_ptr<grpc::Channel> channel;
  std::unique_ptr<qtrade::account::v1::AccountService::Stub> stub;
  bool initialized = false;
};

AccountClient::AccountClient() : impl_(std::make_unique<Impl>()) {}

AccountClient::~AccountClient() { Shutdown(); }

ErrorCode AccountClient::Init(const AccountClientOptions& options) {
  if (impl_->initialized) {
    return ErrorCode::kSystemError;
  }
  if (options.server_address.empty()) {
    return ErrorCode::kInternal;
  }

  impl_->options = options;
  impl_->channel = grpc::CreateChannel(options.server_address, grpc::InsecureChannelCredentials());
  impl_->stub = qtrade::account::v1::AccountService::NewStub(impl_->channel);
  impl_->initialized = true;
  return ErrorCode::kSuccess;
}

void AccountClient::Shutdown() {
  impl_->stub.reset();
  impl_->channel.reset();
  impl_->initialized = false;
}

ErrorCode AccountClient::ResolveCredential(const std::string& account_id,
                                           qtrade::account::v1::ResolveCredentialResponse& response) {
  if (!impl_->initialized || !impl_->stub) {
    return ErrorCode::kNotInitialized;
  }
  if (account_id.empty()) {
    return ErrorCode::kInternal;
  }

  qtrade::account::v1::ResolveCredentialRequest request;
  request.set_engine_id(impl_->options.engine_id);
  request.set_account_id(account_id);

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

  const grpc::Status status = impl_->stub->ResolveCredential(&context, request, &response);
  if (!status.ok()) {
    spdlog::warn("[AccountClient] ResolveCredential failed: {}", status.error_message());
    return ErrorCode::kTimeout;
  }

  spdlog::info("[AccountClient] credential resolved for account={}", account_id);
  return ErrorCode::kSuccess;
}

bool AccountClient::IsInitialized() const { return impl_->initialized; }

}  // namespace qtrade::client
