/// @file      trading_engine_struct.hpp
/// @brief     引擎交易核心模块与支撑 Client 的组合持有（普通 struct）
/// @details   由 TradingEngine 拥有一份；公开成员直接访问，不做 Getter 包装。
///            OrderPipeline 依赖 CMS/Risk/OMS/EMS，成员声明顺序不可乱。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_TRADING_ENGINE_STRUCT_HPP_
#define QTRADE_ENGINE_TRADING_ENGINE_STRUCT_HPP_

#include "qtrade/client/account_client/account_client.hpp"
#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/client/config_client/config_client.hpp"
#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/cms/compliance_manager.hpp"
#include "qtrade/engine/core/order_pipeline.hpp"
#include "qtrade/engine/ems/execution_manager.hpp"
#include "qtrade/engine/oms/order_manager.hpp"
#include "qtrade/engine/position/position_manager.hpp"
#include "qtrade/engine/risk/risk_manager.hpp"

namespace qtrade::engine {

/// @brief 交易核心模块 + gRPC Client 袋子（非单例；一引擎一份）
struct EngineModules {
  /// 合规模块
  cms::ComplianceManager compliance;
  /// 实例风控
  risk::RiskManager risk;
  /// 订单管理
  oms::OrderManager orders;
  /// 执行管理
  ems::ExecutionManager execution;
  /// 账户资金
  account::AccountManager account;
  /// 持仓
  position::PositionManager position;
  /// 发单流水线（须在 compliance/risk/orders/execution 之后）
  OrderPipeline pipeline{compliance, risk, orders, execution};

  /// 配置服务客户端
  client::ConfigClient config_client;
  /// 账户凭证客户端
  client::AccountClient account_client;
  /// 账户硬风控客户端
  client::AccountRiskClient account_risk_client;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_TRADING_ENGINE_STRUCT_HPP_
