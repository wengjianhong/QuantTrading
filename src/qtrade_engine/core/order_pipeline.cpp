/// @file      order_pipeline.cpp
/// @brief     OrderPipeline 发单准入编排实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/order_pipeline.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine {

OrderPipeline::OrderPipeline(cms::ComplianceApi& compliance,
                             oms::OrderApi& orders,
                             ems::ExecutionApi& execution,
                             qtrade::account_risk::IAccountRiskBridge* account_risk_bridge)
  : compliance_(compliance),
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
  qtrade::account_risk::ReleaseRequest request;
  request.account_id = account_id_;
  request.order_id = order_id;
  request.reason = reason;
  const auto result = account_risk_bridge_->Release(request);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("Release failed: order_id={}, code={}", order_id, static_cast<int>(result.error_code));
  }
}

ErrorCode OrderPipeline::Submit(const qtrade::sdk::trader::OrderRequest& request) {
  // 1. 策略级 CMS
  if (const auto rc = compliance_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (request.client_order_id != 0 && orders_.GetOrderByClientId(request.client_order_id).has_value()) {
    return ErrorCode::kSuccess;
  }

  // 2. 账户硬风控预占（跨策略）
  const std::string order_id = orders_.AllocateOrderId();
  if (account_risk_bridge_ != nullptr) {
    qtrade::account_risk::ReserveRequest reserve_request;
    reserve_request.account_id = account_id_;
    reserve_request.order_id = order_id;
    reserve_request.exposure.engine_id = engine_id_;
    reserve_request.exposure.strategy_id = request.strategy_id;
    reserve_request.exposure.instrument_id = request.instrument;
    reserve_request.exposure.price = request.price;
    reserve_request.exposure.quantity = static_cast<std::uint64_t>(request.volume);
    reserve_request.exposure.notional = request.price * static_cast<double>(request.volume);
    reserve_request.exposure.side = request.side;
    reserve_request.expected_policy_version = 0;

    const auto reserve_result = account_risk_bridge_->Reserve(reserve_request);
    const bool reserve_unknown = reserve_result.error_code == ErrorCode::kTimeout ||
                                 (reserve_result.error_code == ErrorCode::kSuccess && reserve_result.data.has_value() &&
                                  reserve_result.data->state == qtrade::account_risk::ReservationState::kUnspecified);
    if (reserve_unknown) {
      const auto query_result = account_risk_bridge_->QueryReservation(account_id_, order_id);
      if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value() ||
          query_result.data->state != qtrade::account_risk::ReservationState::kReserved) {
        return query_result.error_code == ErrorCode::kNotFound ? ErrorCode::kTimeout : query_result.error_code;
      }
    } else if (reserve_result.error_code != ErrorCode::kSuccess || !reserve_result.data.has_value() ||
               reserve_result.data->state != qtrade::account_risk::ReservationState::kReserved) {
      return reserve_result.error_code == ErrorCode::kSuccess ? ErrorCode::kInternalError : reserve_result.error_code;
    }
  }

  // 3. OMS → EMS
  const auto order = orders_.CreateOrder(request, order_id);
  if (!order.has_value()) {
    ReleaseReservation(order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    return ErrorCode::kNotInitialized;
  }
  const std::string& created_order_id = order->order_id;
  const auto lifecycle = orders_.GetLifecycleState(created_order_id);
  if (lifecycle.has_value() && *lifecycle != oms::OrderLifecycleState::kPrepared) {
    return ErrorCode::kSuccess;
  }

  if (const auto rc = orders_.MarkEmsQueued(created_order_id); rc != ErrorCode::kSuccess) {
    ReleaseReservation(created_order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    return rc;
  }

  const auto rc = execution_.Enqueue(*order);
  if (rc != ErrorCode::kSuccess) {
    (void)orders_.RecordSendResult(created_order_id, rc);
    ReleaseReservation(created_order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
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

  qtrade::sdk::trader::CancelOrderRequest request;
  request.order_id = order_id;
  request.broker_order_id = order->broker_order_id;
  const auto result = execution_.EnqueueCancel(request);
  if (result != ErrorCode::kSuccess) {
    (void)orders_.RecordCancelResult(order_id, result);
  }
  return result;
}

}  // namespace qtrade::engine
