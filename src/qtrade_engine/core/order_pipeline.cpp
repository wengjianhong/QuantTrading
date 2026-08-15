/// @file      order_pipeline.cpp
/// @brief     OrderPipeline 发单准入编排实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/order_pipeline.hpp"

namespace qtrade::engine {

OrderPipeline::OrderPipeline(strategy_risk::StrategyRiskApi& compliance,
                             orders::OrderApi& orders,
                             execution::ExecutionApi& execution,
                             account_risk::AccountRiskApi& account_risk)
  : compliance_(compliance), orders_(orders), execution_(execution), account_risk_(account_risk) {}

ErrorCode OrderPipeline::Submit(const qtrade::sdk::trader::OrderRequest& request) {
  // 1. 策略级 CMS
  if (const auto rc = compliance_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (request.client_order_id != 0 && orders_.GetOrderByClientId(request.client_order_id).has_value()) {
    return ErrorCode::kSuccess;
  }

  // 2. 账户硬风控预占（跨策略，同步）
  const std::string order_id = orders_.AllocateOrderId();
  if (const auto rc = account_risk_.Reserve(request, order_id); rc != ErrorCode::kSuccess) {
    return rc;
  }

  // 3. OMS → EMS
  const auto order = orders_.CreateOrder(request, order_id);
  if (!order.has_value()) {
    account_risk_.Release(order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    return ErrorCode::kNotInitialized;
  }
  const std::string& created_order_id = order->order_id;
  const auto lifecycle = orders_.GetLifecycleState(created_order_id);
  if (lifecycle.has_value() && *lifecycle != orders::OrderLifecycleState::kPrepared) {
    return ErrorCode::kSuccess;
  }

  if (const auto rc = orders_.MarkEmsQueued(created_order_id); rc != ErrorCode::kSuccess) {
    account_risk_.Release(created_order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    return rc;
  }

  const auto rc = execution_.Enqueue(*order);
  if (rc != ErrorCode::kSuccess) {
    (void)orders_.RecordSendResult(created_order_id, rc);
    account_risk_.Release(created_order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
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
