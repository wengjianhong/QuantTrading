#include "qtrade/engine/order_pipeline.hpp"

#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/client/log_client/log_client.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.pb.h>

namespace qtrade::engine {

OrderPipeline::OrderPipeline(cms::ComplianceManager& compliance,
                             risk::RiskManager& risk_manager,
                             oms::OrderManager& order_manager,
                             ems::ExecutionManager& execution_manager,
                             qtrade::client::AccountRiskClient* account_risk_client)
  : compliance_(compliance),
    risk_manager_(risk_manager),
    order_manager_(order_manager),
    execution_manager_(execution_manager),
    account_risk_client_(account_risk_client) {}

void OrderPipeline::SetAccountRiskClient(qtrade::client::AccountRiskClient* account_risk_client) {
  account_risk_client_ = account_risk_client;
}

void OrderPipeline::SetLogClient(qtrade::client::LogClient* log_client) {
  log_client_ = log_client;
}

ErrorCode OrderPipeline::Submit(const qtrade_sdk::trader::OrderRequest& request) {
  if (log_client_ != nullptr && log_client_->IsAuditHalted()) {
    return ErrorCode::kInternal;
  }
  if (const auto rc = compliance_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (const auto rc = risk_manager_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  const std::string order_id = order_manager_.AllocateOrderId();
  if (account_risk_client_ != nullptr) {
    qtrade::account_risk::v1::ReserveOrderResponse response;
    if (const auto rc = account_risk_client_->ReserveOrder(order_id, request, 0, response);
        rc != ErrorCode::kSuccess || response.decision() != qtrade::account_risk::v1::ReserveOrderResponse::APPROVED) {
      return rc == ErrorCode::kSuccess ? ErrorCode::kInternal : rc;
    }
  }
  const auto order = order_manager_.CreateOrder(request, order_id);
  if (!order.has_value()) {
    if (account_risk_client_ != nullptr) {
      qtrade::account_risk::v1::ReleaseOrderResponse ignored;
      (void)account_risk_client_->ReleaseOrder(
        order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED, ignored);
    }
    return ErrorCode::kNotInitialized;
  }
  const auto rc = execution_manager_.Enqueue(*order);
  if (rc != ErrorCode::kSuccess && account_risk_client_ != nullptr) {
    qtrade::account_risk::v1::ReleaseOrderResponse ignored;
    (void)account_risk_client_->ReleaseOrder(
      order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED, ignored);
  }
  return rc;
}

}  // namespace qtrade::engine
