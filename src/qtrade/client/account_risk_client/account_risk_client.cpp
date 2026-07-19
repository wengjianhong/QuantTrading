/// @file      account_risk_client.cpp
/// @brief     AccountRiskClient gRPC 调用实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/client/account_risk_client/account_risk_client.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.grpc.pb.h>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <grpcpp/grpcpp.h>

#include <chrono>

namespace qtrade::client {

struct AccountRiskClient::Impl {
  /// 初始化选项快照
  AccountRiskClientOptions options;
  /// gRPC 通道
  std::shared_ptr<grpc::Channel> channel;
  /// AccountRiskService stub
  std::unique_ptr<qtrade::account_risk::v1::AccountRiskService::Stub> stub;
};

AccountRiskClient::AccountRiskClient() : impl_(std::make_unique<Impl>()) {}
AccountRiskClient::~AccountRiskClient() {
  Shutdown();
}

ErrorCode AccountRiskClient::Init(const AccountRiskClientOptions& options) {
  if (impl_->stub || options.server_address.empty() || options.tenant_id.empty() || options.account_id.empty() ||
      options.timeout_ms <= 0) {
    return ErrorCode::kInternal;
  }
  impl_->options = options;
  impl_->channel = grpc::CreateChannel(options.server_address, grpc::InsecureChannelCredentials());
  impl_->stub = qtrade::account_risk::v1::AccountRiskService::NewStub(impl_->channel);
  return ErrorCode::kSuccess;
}

void AccountRiskClient::Shutdown() {
  impl_->stub.reset();
  impl_->channel.reset();
}

bool AccountRiskClient::IsInitialized() const {
  return impl_->stub != nullptr;
}

ErrorCode AccountRiskClient::ReserveOrder(const std::string& order_id,
                                          const qtrade_sdk::trader::OrderRequest& request,
                                          std::uint64_t risk_config_version,
                                          qtrade::account_risk::v1::ReserveOrderResponse& response) {
  if (!IsInitialized() || order_id.empty()) {
    return ErrorCode::kNotInitialized;
  }
  qtrade::account_risk::v1::ReserveOrderRequest rpc_request;
  rpc_request.set_tenant_id(impl_->options.tenant_id);
  rpc_request.set_account_id(impl_->options.account_id);
  rpc_request.set_risk_config_version(risk_config_version);
  auto* intent = rpc_request.mutable_intent();
  intent->set_order_id(order_id);
  intent->set_engine_id(impl_->options.engine_id);
  intent->set_instrument_id(request.instrument);
  intent->set_price(request.price);
  intent->set_quantity(request.volume);
  intent->set_estimated_notional(request.price * static_cast<double>(request.volume));
  intent->set_side(std::to_string(static_cast<int>(request.side)));
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(impl_->options.timeout_ms));
  const grpc::Status status = impl_->stub->ReserveOrder(&context, rpc_request, &response);
  return status.ok() ? ErrorCode::kSuccess : ErrorCode::kTimeout;
}

ErrorCode AccountRiskClient::GetReservation(
  const std::string& order_id,
  qtrade::account_risk::v1::Reservation& reservation) {
  if (!IsInitialized() || order_id.empty()) {
    return ErrorCode::kNotInitialized;
  }
  qtrade::account_risk::v1::GetReservationRequest request;
  request.set_tenant_id(impl_->options.tenant_id);
  request.set_account_id(impl_->options.account_id);
  request.set_order_id(order_id);
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(impl_->options.timeout_ms));
  qtrade::account_risk::v1::GetReservationResponse response;
  const grpc::Status status = impl_->stub->GetReservation(&context, request, &response);
  if (status.ok()) {
    reservation = response.reservation();
    return ErrorCode::kSuccess;
  }
  return status.error_code() == grpc::StatusCode::NOT_FOUND ? ErrorCode::kNotFound : ErrorCode::kTimeout;
}

ErrorCode AccountRiskClient::ReleaseOrder(const std::string& order_id,
                                          int reason,
                                          qtrade::account_risk::v1::ReleaseOrderResponse& response) {
  if (!IsInitialized() || order_id.empty()) {
    return ErrorCode::kNotInitialized;
  }
  qtrade::account_risk::v1::ReleaseOrderRequest request;
  request.set_tenant_id(impl_->options.tenant_id);
  request.set_account_id(impl_->options.account_id);
  request.set_order_id(order_id);
  request.set_reason(static_cast<qtrade::account_risk::v1::ReleaseOrderRequest::Reason>(reason));
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(impl_->options.timeout_ms));
  const grpc::Status status = impl_->stub->ReleaseOrder(&context, request, &response);
  return status.ok() ? ErrorCode::kSuccess : ErrorCode::kTimeout;
}

}  // namespace qtrade::client
