/// @file      order_pipeline.cpp
/// @brief     OrderPipeline 发单 A 段编排实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/order_pipeline.hpp"

namespace qtrade::engine {

OrderPipeline::OrderPipeline(compliance::ComplianceApi& compliance,
                             strategy_risk::StrategyRiskApi& strategy_risk,
                             instance_risk::InstanceRiskApi& instance_risk,
                             orders::OrderApi& orders,
                             execution::ExecutionApi& execution,
                             OrderIntentQueue& intent_queue)
  : compliance_(compliance),
    strategy_risk_(strategy_risk),
    instance_risk_(instance_risk),
    orders_(orders),
    execution_(execution),
    intent_queue_(intent_queue) {}

ErrorCode OrderPipeline::Submit(const qtrade::sdk::trader::OrderRequest& request) {
  if (const auto rc = compliance_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (const auto rc = strategy_risk_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (const auto rc = instance_risk_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (request.client_order_id != 0 && orders_.GetOrderByClientId(request.client_order_id).has_value()) {
    return ErrorCode::kSuccess;
  }

  OrderIntent intent;
  intent.request = request;
  return intent_queue_.Enqueue(std::move(intent));
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
