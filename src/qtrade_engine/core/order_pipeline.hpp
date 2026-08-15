/// @file      order_pipeline.hpp
/// @brief     发单准入流水线：CMS(按策略) → E 段预占 → OMS → EMS
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_

#include "qtrade/engine/account_risk/account_risk_api.hpp"
#include "qtrade/engine/execution/execution_api.hpp"
#include "qtrade/engine/instance_risk/instance_risk_api.hpp"
#include "qtrade/engine/orders/order_api.hpp"
#include "qtrade/engine/strategy_risk/strategy_risk_api.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <string>

namespace qtrade::engine {

/// @brief 发单准入编排：策略风控、实例风控、账户预占、OMS 与 EMS
class OrderPipeline {
 public:
  OrderPipeline(strategy_risk::StrategyRiskApi& compliance,
                instance_risk::InstanceRiskApi& instance_risk,
                orders::OrderApi& orders,
                execution::ExecutionApi& execution,
                account_risk::AccountRiskApi& account_risk);

  ErrorCode Submit(const qtrade::sdk::trader::OrderRequest& request);

  ErrorCode SubmitBatch(const qtrade::strategy::OrderBatch& batch);

  ErrorCode Cancel(const std::string& order_id);

 private:
  strategy_risk::StrategyRiskApi& compliance_;
  instance_risk::InstanceRiskApi& instance_risk_;
  orders::OrderApi& orders_;
  execution::ExecutionApi& execution_;
  account_risk::AccountRiskApi& account_risk_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
