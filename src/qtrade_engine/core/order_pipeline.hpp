/// @file      order_pipeline.hpp
/// @brief     发单准入流水线：CMS(按策略) → E 段预占 → OMS → EMS
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_

#include "qtrade/engine/cms/compliance_api.hpp"
#include "qtrade/engine/account_risk/account_risk_api.hpp"
#include "qtrade/engine/ems/execution_api.hpp"
#include "qtrade/engine/oms/order_api.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <string>

namespace qtrade::engine {

/// @brief A 段后准入编排：策略合规、账户预占、OMS 落单与 EMS 报送
class OrderPipeline {
 public:
  OrderPipeline(cms::ComplianceApi& compliance,
                oms::OrderApi& orders,
                ems::ExecutionApi& execution,
                account_risk::AccountRiskApi& account_risk);

  ErrorCode Submit(const qtrade::sdk::trader::OrderRequest& request);

  ErrorCode SubmitBatch(const qtrade::strategy::OrderBatch& batch);

  ErrorCode Cancel(const std::string& order_id);

 private:
  cms::ComplianceApi& compliance_;
  oms::OrderApi& orders_;
  ems::ExecutionApi& execution_;
  account_risk::AccountRiskApi& account_risk_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
