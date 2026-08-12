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
                             qtrade::account_risk::IAccountRiskBridge* account_risk_bridge)
  : compliance_(compliance),
    risk_(risk),
    orders_(orders),
    execution_(execution),
    account_risk_bridge_(account_risk_bridge) {}

void OrderPipeline::SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* account_risk_bridge) {
  account_risk_bridge_ = account_risk_bridge;
}

void OrderPipeline::SetAccountRiskIdentity(std::string account_id, std::string engine_id) {
  account_id_ = std::move(account_id);
  engine_id_ = std::move(engine_id);
}

void OrderPipeline::ReleaseReservation(const std::string& order_id, qtrade::account_risk::ReleaseReason reason) {
  if (account_risk_bridge_ == nullptr || order_id.empty()) {
    return;
  }
  const auto result = account_risk_bridge_->ReleaseOrder(account_id_, order_id, reason, 0.0, 0.0);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("ReleaseOrder failed: order_id={}, code={}", order_id, static_cast<int>(result.error_code));
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
  if (account_risk_bridge_ != nullptr) {
    qtrade::account_risk::OrderIntent intent;
    intent.order_id = order_id;
    intent.engine_id = engine_id_;
    intent.instrument_id = request.instrument;
    intent.price = request.price;
    intent.quantity = static_cast<std::uint64_t>(request.volume);
    intent.estimated_notional = request.price * static_cast<double>(request.volume);
    intent.side = std::to_string(static_cast<int>(request.side));

    const auto reserve_result = account_risk_bridge_->ReserveOrder(account_id_, intent, risk_.Version(), 0);
    const bool reserve_unknown = reserve_result.error_code == ErrorCode::kTimeout ||
                                 (reserve_result.error_code == ErrorCode::kSuccess && reserve_result.data.has_value() &&
                                  reserve_result.data->decision == qtrade::account_risk::ReserveDecision::kUnknown);
    if (reserve_unknown) {
      const auto query_result = account_risk_bridge_->QueryReservation(account_id_, order_id);
      if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value() ||
          query_result.data->status != "reserved") {
        return query_result.error_code == ErrorCode::kNotFound ? ErrorCode::kTimeout : query_result.error_code;
      }
    } else if (reserve_result.error_code != ErrorCode::kSuccess || !reserve_result.data.has_value() ||
               reserve_result.data->decision != qtrade::account_risk::ReserveDecision::kApproved) {
      return reserve_result.error_code == ErrorCode::kSuccess ? ErrorCode::kInternalError : reserve_result.error_code;
    }
  }

  const auto order = orders_.CreateOrder(request, order_id);
  if (!order.has_value()) {
    ReleaseReservation(order_id, qtrade::account_risk::ReleaseReason::kEmsEnqueueFailed);
    return ErrorCode::kNotInitialized;
  }
  const std::string& created_order_id = order->order_id;
  const auto lifecycle = orders_.GetLifecycleState(created_order_id);
  if (lifecycle.has_value() && *lifecycle != oms::OrderLifecycleState::kPrepared) {
    return ErrorCode::kSuccess;
  }

  if (const auto rc = orders_.MarkEmsQueued(created_order_id); rc != ErrorCode::kSuccess) {
    ReleaseReservation(created_order_id, qtrade::account_risk::ReleaseReason::kEmsEnqueueFailed);
    return rc;
  }

  const auto rc = execution_.Enqueue(*order);
  if (rc != ErrorCode::kSuccess) {
    (void)orders_.RecordSendResult(created_order_id, rc);
    ReleaseReservation(created_order_id, qtrade::account_risk::ReleaseReason::kEmsEnqueueFailed);
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
  const auto order = orders_.GetOrder(order_id);
  if (!order.has_value()) {
    return ErrorCode::kNotFound;
  }
  if (const auto result = orders_.CancelOrder(order_id); result != ErrorCode::kSuccess) {
    return result;
  }

  qtrade_sdk::trader::CancelOrderRequest request;
  request.order_id = order_id;
  request.broker_order_id = order->broker_order_id;
  const auto result = execution_.EnqueueCancel(request);
  if (result != ErrorCode::kSuccess) {
    (void)orders_.RecordCancelResult(order_id, result);
  }
  return result;
}

}  // namespace qtrade::engine
