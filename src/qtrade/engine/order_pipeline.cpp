#include "qtrade/engine/order_pipeline.hpp"

namespace qtrade::engine {

OrderPipeline::OrderPipeline(cms::ComplianceManager& compliance,
                             risk::RiskManager& risk_manager,
                             oms::OrderManager& order_manager,
                             ems::ExecutionManager& execution_manager)
  : compliance_(compliance),
    risk_manager_(risk_manager),
    order_manager_(order_manager),
    execution_manager_(execution_manager) {}

ErrorCode OrderPipeline::Submit(const qtrade_sdk::trader::OrderRequest& request) {
  if (const auto rc = compliance_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (const auto rc = risk_manager_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  const auto order = order_manager_.CreateOrder(request);
  if (!order.has_value()) {
    return ErrorCode::kNotInitialized;
  }
  return execution_manager_.Enqueue(*order);
}

}  // namespace qtrade::engine
