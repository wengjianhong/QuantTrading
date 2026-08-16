/// @file      lane_event_handler.hpp
/// @brief     Lane-T 引擎侧回报处理：订单/成交写入 OMS、账户、持仓
/// @details   须在 StrategyEventDispatcher 之前 Register，保证策略读到已更新状态。
///            不看 READY；Frozen 仍处理回报。终态预占释放经 AccountRiskApi 发出。
///            本类是 Lane 出站消费者，不是 SDK 回调入口。
/// @author    wengjianhong
/// @date      2026-08-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_LANE_EVENT_HANDLER_HPP_
#define QTRADE_ENGINE_LANE_EVENT_HANDLER_HPP_

#include "qtrade/engine/account/account_api.hpp"
#include "qtrade/engine/account_risk/account_risk_api.hpp"
#include "qtrade/engine/events/event_lanes.hpp"
#include "qtrade/engine/orders/order_api.hpp"
#include "qtrade/engine/positions/position_api.hpp"

#include <qtrade/sdk/trader/trader_struct.hpp>

namespace qtrade::engine {

/// @brief Lane-T 上的引擎侧 Order / Trade 订阅者
class LaneEventHandler {
 public:
  /// @brief 绑定 OMS / 账户 / 持仓 / 硬风控接口（不拥有）
  /// @param orders 订单管理接口
  /// @param account 账户资金接口
  /// @param position 持仓接口
  /// @param account_risk 账户硬风控接口
  LaneEventHandler(orders::OrderApi& orders,
                   account::AccountApi& account,
                   positions::PositionApi& position,
                   account_risk::AccountRiskApi& account_risk);

  /// @brief 向 Lane-T 注册 Order/Trade 回调（Stop 清空后每次 Start 须再调）
  /// @param event_lanes 事件通道
  void Register(events::EventLanes& event_lanes);

  /// @brief 处理订单回报：OMS → 本地快照更新账户冻结 → 拒单/撤单释放预占
  /// @param order 柜台订单回报
  void OnOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 处理成交回报：OMS → 账户现金流 → 持仓 → 全成释放预占
  /// @param trade 柜台成交回报
  void OnTrade(const qtrade::sdk::trader::Trade& trade);

 private:
  /// OMS
  orders::OrderApi& orders_api_;
  /// 账户资金视图
  account::AccountApi& account_api_;
  /// 持仓视图
  positions::PositionApi& position_api_;
  /// 账户硬风控模块间接口
  account_risk::AccountRiskApi& account_risk_api_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_LANE_EVENT_HANDLER_HPP_
