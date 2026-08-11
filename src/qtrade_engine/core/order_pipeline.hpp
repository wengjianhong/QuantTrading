/// @file      order_pipeline.hpp
/// @brief     发单准入流水线：CMS → Risk → E 段预占 → OMS → EMS
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_

#include "qtrade/engine/cms/compliance_api.hpp"
#include "qtrade/engine/ems/execution_api.hpp"
#include "qtrade/engine/oms/order_api.hpp"
#include "qtrade/engine/risk/risk_api.hpp"

#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/strategy/strategy.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <string>

namespace qtrade::engine {

/// @brief A 段后准入编排：合规、实例风控、账户预占、OMS 落单与 EMS 报送
class OrderPipeline {
 public:
  OrderPipeline(cms::ComplianceApi& compliance,
                risk::RiskApi& risk,
                oms::OrderApi& orders,
                ems::ExecutionApi& execution,
                qtrade::account_risk::IAccountRiskBridge* account_risk_bridge = nullptr);

  void SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* account_risk_bridge);

  void SetAccountRiskIdentity(std::string account_id, std::string engine_id);

  ErrorCode Submit(const qtrade_sdk::trader::OrderRequest& request);

  ErrorCode SubmitBatch(const qtrade::strategy::OrderBatch& batch);

  ErrorCode Cancel(const std::string& order_id);

 private:
  void ReleaseReservation(const std::string& order_id, qtrade::account_risk::ReleaseReason reason);

  cms::ComplianceApi& compliance_;
  risk::RiskApi& risk_;
  oms::OrderApi& orders_;
  ems::ExecutionApi& execution_;
  qtrade::account_risk::IAccountRiskBridge* account_risk_bridge_ = nullptr;
  std::string account_id_;
  std::string engine_id_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
