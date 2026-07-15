#ifndef QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_

#include "qtrade/engine/cms/compliance_manager.hpp"
#include "qtrade/engine/ems/execution_manager.hpp"
#include "qtrade/engine/oms/order_manager.hpp"
#include "qtrade/engine/risk/risk_manager.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

namespace qtrade::engine {

/// @brief A 段发单编排：CMS → Risk → OMS → EMS。
class OrderPipeline {
 public:
  OrderPipeline(cms::ComplianceManager& compliance,
                risk::RiskManager& risk_manager,
                oms::OrderManager& order_manager,
                ems::ExecutionManager& execution_manager);

  ErrorCode Submit(const qtrade_sdk::trader::OrderRequest& request);

 private:
  cms::ComplianceManager& compliance_;
  risk::RiskManager& risk_manager_;
  oms::OrderManager& order_manager_;
  ems::ExecutionManager& execution_manager_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
