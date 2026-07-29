/// @file      trading_engine_struct.hpp
/// @brief     引擎交易核心模块与支撑 Client 的组合持有（普通 struct）
/// @details   EngineModules 持有各 XxxManager 实现；跨模块协作只通过 XxxApi。
///            SupportClients：外部 gRPC Client。由 TradingEngine 各持有一份。
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

/// @brief 外部支撑服务 gRPC Client（非单例；一引擎一份）
struct SupportClients {
  /// 配置服务客户端
  client::ConfigClient config_client;
  /// 账户凭证客户端
  client::AccountClient account_client;
  /// 账户硬风控客户端
  client::AccountRiskClient account_risk_client;
};

/// @brief 交易核心模块袋子（持有 Manager 实现；跨模块走 XxxApi）
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
  /// 发单流水线（须在 compliance/risk/orders/execution 之后；构造参数绑 XxxApi）
  OrderPipeline pipeline{compliance, risk, orders, execution};
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_TRADING_ENGINE_STRUCT_HPP_
