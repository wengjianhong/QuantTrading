/// @file      account_client.cpp
/// @brief     AccountService gRPC 客户端实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/client/account_client/account_client.hpp"

#include <qtrade/proto/account/v1/account.grpc.pb.h>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <chrono>

namespace qtrade::client {
namespace {

[[nodiscard]] std::chrono::system_clock::time_point DeadlineFrom(const qtrade::common::config::ServiceConfig& cfg) {
  const int timeout_ms = cfg.timeout_ms > 0 ? cfg.timeout_ms : 5000;
  return std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms);
}

}  // namespace

struct AccountClient::Impl {
  AccountClientOptions options;
  std::shared_ptr<grpc::Channel> channel;
  std::unique_ptr<qtrade::account::v1::AccountService::Stub> stub;
  bool initialized = false;
};

AccountClient::AccountClient() : impl_(std::make_unique<Impl>()) {}

AccountClient::~AccountClient() {
  Shutdown();
}

ErrorCode AccountClient::Init(const AccountClientOptions& options) {
  if (impl_->initialized) {
    return ErrorCode::kSystemError;
  }
  if (options.service_config.host.empty() || options.service_config.port <= 0) {
    return ErrorCode::kInternalError;
  }
  impl_->options = options;
  impl_->channel = grpc::CreateChannel(options.service_config.Address(), grpc::InsecureChannelCredentials());
  impl_->stub = qtrade::account::v1::AccountService::NewStub(impl_->channel);
  impl_->initialized = true;
  return ErrorCode::kSuccess;
}

bool AccountClient::IsInitialized() const {
  return impl_->initialized;
}

void AccountClient::Shutdown() {
  impl_->stub.reset();
  impl_->channel.reset();
  impl_->initialized = false;
}

ErrorCode AccountClient::GetCredential(const qtrade::account::v1::GetCredentialRequest& request,
                                       qtrade::account::v1::GetCredentialResponse& response) {
  if (!impl_->initialized || !impl_->stub) {
    return ErrorCode::kNotInitialized;
  }
  grpc::ClientContext context;
  context.set_deadline(DeadlineFrom(impl_->options.service_config));
  const grpc::Status status = impl_->stub->GetCredential(&context, request, &response);
  if (!status.ok()) {
    spdlog::warn("[AccountClient] GetCredential failed: {}", status.error_message());
    return ErrorCode::kTimeout;
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::client
