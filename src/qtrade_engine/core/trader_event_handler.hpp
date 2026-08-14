/// @file      trader_event_handler.hpp
/// @brief     Lane-T 引擎侧回报处理：订单/成交写入 OMS、账户、持仓并释放终态预占
/// @details   须在 StrategyEventDispatcher 之前 Register，保证策略读到已更新状态。
///            不看 READY；Frozen 仍处理回报。发送失败释放预占仍由 OrderPipeline 负责。
/// @author    wengjianhong
/// @date      2026-08-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_TRADER_EVENT_HANDLER_HPP_
#define QTRADE_ENGINE_TRADER_EVENT_HANDLER_HPP_

#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/oms/order_api.hpp"
#include "qtrade/engine/position/position_manager.hpp"

#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <string>

namespace qtrade::engine {

/// @brief Lane-T 上的引擎侧 Order / Trade 订阅者
class TraderEventHandler {
 public:
  /// @brief 绑定 OMS / 账户 / 持仓依赖（不拥有）
  /// @param orders OMS 模块间接口
  /// @param account 账户资金视图
  /// @param position 持仓视图
  /// @param account_risk_bridge 账户硬风控桥；可空
  TraderEventHandler(oms::OrderApi& orders,
                    account::AccountManager& account,
                    position::PositionManager& position,
                    qtrade::account_risk::IAccountRiskBridge* account_risk_bridge = nullptr);

  /// @brief 注入或清除账户硬风控桥
  /// @param account_risk_bridge 桥接指针；可空
  void SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* account_risk_bridge);

  /// @brief 设置 Release 所用账户标识
  /// @param account_id 交易账户号
  void SetAccountRiskIdentity(std::string account_id);

  /// @brief 向 Lane-T 注册 Order/Trade 回调（Stop 清空后每次 Start 须再调）
  /// @param event_lanes 事件通道
  void Register(event_bus::EventLanes& event_lanes);

  /// @brief 处理订单回报：OMS → 本地快照更新账户冻结 → 拒单/撤单释放预占
  /// @param order 柜台订单回报
  void OnOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 处理成交回报：OMS → 账户现金流 → 持仓 → 全成释放预占
  /// @param trade 柜台成交回报
  void OnTrade(const qtrade::sdk::trader::Trade& trade);

 private:
  /// @brief 尽力调用 account-risk Release（无本地 outbox）
  /// @param order_id 委托 ID
  /// @param reason 释放原因
  void ReleaseReservation(const std::string& order_id, qtrade::account_risk::ReleaseReason reason);

  /// OMS
  oms::OrderApi& orders_;
  /// 账户
  account::AccountManager& account_;
  /// 持仓
  position::PositionManager& position_;
  /// 账户硬风控桥（非拥有）
  qtrade::account_risk::IAccountRiskBridge* account_risk_bridge_ = nullptr;
  /// Release 用账户号
  std::string account_id_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_TRADER_EVENT_HANDLER_HPP_
