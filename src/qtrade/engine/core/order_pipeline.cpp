/// @file      order_pipeline.cpp
/// @brief     OrderPipeline 发单准入编排实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/order_pipeline.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine {

OrderPipeline::OrderPipeline(cms::ComplianceApi& compliance,
                             risk::RiskApi& risk,
                             oms::OrderApi& orders,
                             ems::ExecutionApi& execution,
                             qtrade::client::AccountRiskClient* account_risk_client)
  : compliance_(compliance),
    risk_(risk),
    orders_(orders),
    execution_(execution),
    account_risk_client_(account_risk_client) {}

void OrderPipeline::SetAccountRiskClient(qtrade::client::AccountRiskClient* account_risk_client) {
  account_risk_client_ = account_risk_client;
}

void OrderPipeline::SetAccountRiskIdentity(std::string tenant_id, std::string account_id, std::string engine_id) {
  tenant_id_ = std::move(tenant_id);
  account_id_ = std::move(account_id);
  engine_id_ = std::move(engine_id);
}

void OrderPipeline::ReleaseReservation(const std::string& order_id,
                                       qtrade::account_risk::v1::ReleaseOrderRequest::Reason reason) {
  if (account_risk_client_ == nullptr || order_id.empty()) {
    return;
  }
  qtrade::account_risk::v1::ReleaseOrderRequest request;
  request.set_tenant_id(tenant_id_);
  request.set_account_id(account_id_);
  request.set_order_id(order_id);
  request.set_reason(reason);
  qtrade::account_risk::v1::ReleaseOrderResponse response;
  if (const auto rc = account_risk_client_->ReleaseOrder(request, response); rc != ErrorCode::kSuccess) {
    spdlog::warn("ReleaseOrder failed: order_id={}, code={}", order_id, static_cast<int>(rc));
  }
}

ErrorCode OrderPipeline::Submit(const qtrade_sdk::trader::OrderRequest& request) {
  if (const auto rc = compliance_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (const auto rc = risk_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (request.client_order_id != 0 && orders_.GetOrderByClientId(request.client_order_id).has_value()) {
    return ErrorCode::kSuccess;
  }

  const std::string order_id = orders_.AllocateOrderId();
  if (account_risk_client_ != nullptr) {
    qtrade::account_risk::v1::ReserveOrderRequest reserve_request;
    reserve_request.set_tenant_id(tenant_id_);
    reserve_request.set_account_id(account_id_);
    reserve_request.set_risk_config_version(risk_.Version());
    auto* intent = reserve_request.mutable_intent();
    intent->set_order_id(order_id);
    intent->set_engine_id(engine_id_);
    intent->set_instrument_id(request.instrument);
    intent->set_price(request.price);
    intent->set_quantity(static_cast<std::uint64_t>(request.volume));
    intent->set_estimated_notional(request.price * static_cast<double>(request.volume));
    intent->set_side(std::to_string(static_cast<int>(request.side)));

    qtrade::account_risk::v1::ReserveOrderResponse response;
    const auto reserve_result = account_risk_client_->ReserveOrder(reserve_request, response);
    const bool reserve_unknown = reserve_result == ErrorCode::kTimeout ||
                                 (reserve_result == ErrorCode::kSuccess &&
                                  response.decision() == qtrade::account_risk::v1::ReserveOrderResponse::UNKNOWN);
    if (reserve_unknown) {
      qtrade::account_risk::v1::GetReservationRequest query_request;
      query_request.set_tenant_id(tenant_id_);
      query_request.set_account_id(account_id_);
      query_request.set_order_id(order_id);
      qtrade::account_risk::v1::GetReservationResponse query_response;
      const auto query_result = account_risk_client_->GetReservation(query_request, query_response);
      if (query_result != ErrorCode::kSuccess || query_response.reservation().status() != "reserved") {
        return query_result == ErrorCode::kNotFound ? ErrorCode::kTimeout : query_result;
      }
    } else if (reserve_result != ErrorCode::kSuccess ||
               response.decision() != qtrade::account_risk::v1::ReserveOrderResponse::APPROVED) {
      return reserve_result == ErrorCode::kSuccess ? ErrorCode::kInternalError : reserve_result;
    }
  }

  const auto order = orders_.CreateOrder(request, order_id);
  if (!order.has_value()) {
    ReleaseReservation(order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
    return ErrorCode::kNotInitialized;
  }
  const std::string& created_order_id = order->order_id;
  const auto lifecycle = orders_.GetLifecycleState(created_order_id);
  if (lifecycle.has_value() && *lifecycle != oms::OrderLifecycleState::kPrepared) {
    return ErrorCode::kSuccess;
  }

  if (const auto rc = orders_.MarkEmsQueued(created_order_id); rc != ErrorCode::kSuccess) {
    ReleaseReservation(created_order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
    return rc;
  }

  const auto rc = execution_.Enqueue(*order);
  if (rc != ErrorCode::kSuccess) {
    (void)orders_.RecordSendResult(created_order_id, rc);
    ReleaseReservation(created_order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
  }
  return rc;
}

ErrorCode OrderPipeline::SubmitBatch(const qtrade::strategy::OrderBatch& batch) {
  ErrorCode last = ErrorCode::kSuccess;
  for (const auto& request : batch.order_requests) {
    last = Submit(request);
    if (last != ErrorCode::kSuccess) {
      return last;
    }
  }
  return last;
}

ErrorCode OrderPipeline::Cancel(const std::string& order_id) {
  // 1. OMS 标记撤单意图
  const auto order = orders_.GetOrder(order_id);
  if (!order.has_value()) {
    return ErrorCode::kNotFound;
  }
  if (const auto result = orders_.CancelOrder(order_id); result != ErrorCode::kSuccess) {
    return result;
  }

  // 2. 入队 EMS 撤单；失败回写撤单结果
  qtrade_sdk::trader::CancelOrderRequest request;
  request.order_id = order_id;
  request.order_emt_id = order->order_emt_id;
  const auto result = execution_.EnqueueCancel(request);
  if (result != ErrorCode::kSuccess) {
    (void)orders_.RecordCancelResult(order_id, result);
  }
  return result;
}

}  // namespace qtrade::engine
