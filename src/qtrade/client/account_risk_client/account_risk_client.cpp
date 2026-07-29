/// @file      account_risk_client.cpp
/// @brief     AccountRiskService gRPC 客户端实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/client/account_risk_client/account_risk_client.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.grpc.pb.h>

#include <grpcpp/grpcpp.h>

#include <chrono>

namespace qtrade::client {
namespace {

[[nodiscard]] std::chrono::system_clock::time_point DeadlineFrom(const qtrade::common::config::ServiceConfig& cfg) {
  const int timeout_ms = cfg.timeout_ms > 0 ? cfg.timeout_ms : 3;
  return std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms);
}

}  // namespace

struct AccountRiskClient::Impl {
  AccountRiskClientOptions options;
  std::shared_ptr<grpc::Channel> channel;
  std::unique_ptr<qtrade::account_risk::v1::AccountRiskService::Stub> stub;
};

AccountRiskClient::AccountRiskClient() : impl_(std::make_unique<Impl>()) {}

AccountRiskClient::~AccountRiskClient() {
  Shutdown();
}

ErrorCode AccountRiskClient::Init(const AccountRiskClientOptions& options) {
  if (impl_->stub || options.service_config.host.empty() || options.service_config.port <= 0) {
    return ErrorCode::kInternalError;
  }
  impl_->options = options;
  if (impl_->options.service_config.timeout_ms <= 0) {
    impl_->options.service_config.timeout_ms = 3;
  }
  impl_->channel = grpc::CreateChannel(options.service_config.Address(), grpc::InsecureChannelCredentials());
  impl_->stub = qtrade::account_risk::v1::AccountRiskService::NewStub(impl_->channel);
  return ErrorCode::kSuccess;
}

bool AccountRiskClient::IsInitialized() const {
  return impl_->stub != nullptr;
}

void AccountRiskClient::Shutdown() {
  impl_->stub.reset();
  impl_->channel.reset();
}

ErrorCode AccountRiskClient::ReserveOrder(const qtrade::account_risk::v1::ReserveOrderRequest& request,
                                          qtrade::account_risk::v1::ReserveOrderResponse& response) {
  if (!IsInitialized()) {
    return ErrorCode::kNotInitialized;
  }
  grpc::ClientContext context;
  context.set_deadline(DeadlineFrom(impl_->options.service_config));
  const grpc::Status status = impl_->stub->ReserveOrder(&context, request, &response);
  return status.ok() ? ErrorCode::kSuccess : ErrorCode::kTimeout;
}

ErrorCode AccountRiskClient::GetReservation(const qtrade::account_risk::v1::GetReservationRequest& request,
                                            qtrade::account_risk::v1::GetReservationResponse& response) {
  if (!IsInitialized()) {
    return ErrorCode::kNotInitialized;
  }
  grpc::ClientContext context;
  context.set_deadline(DeadlineFrom(impl_->options.service_config));
  const grpc::Status status = impl_->stub->GetReservation(&context, request, &response);
  if (status.ok()) {
    return ErrorCode::kSuccess;
  }
  return status.error_code() == grpc::StatusCode::NOT_FOUND ? ErrorCode::kNotFound : ErrorCode::kTimeout;
}

ErrorCode AccountRiskClient::ReleaseOrder(const qtrade::account_risk::v1::ReleaseOrderRequest& request,
                                          qtrade::account_risk::v1::ReleaseOrderResponse& response) {
  if (!IsInitialized()) {
    return ErrorCode::kNotInitialized;
  }
  grpc::ClientContext context;
  context.set_deadline(DeadlineFrom(impl_->options.service_config));
  const grpc::Status status = impl_->stub->ReleaseOrder(&context, request, &response);
  return status.ok() ? ErrorCode::kSuccess : ErrorCode::kTimeout;
}

}  // namespace qtrade::client
