/// @file      strategy_event_dispatcher.hpp
/// @brief     策略事件分发器：订阅 EventLanes 并按品种路由到策略实例
/// @details   路由表与广播列表在 SetRouting 时拷贝自持；Init 后冻结，不依赖 Manager 内部状态。
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_
#define QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_

#include "qtrade/engine/events/event_lanes.hpp"
#include "qtrade/strategy/strategy.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::strategies {

/// @brief 按 instrument 路由行情/回报到策略；无路由时广播
class StrategyEventDispatcher {
 public:
  /// @brief 仅绑定事件通道
  /// @param event_lanes Lane-Q / Lane-T
  explicit StrategyEventDispatcher(events::EventLanes& event_lanes);

  /// @brief 停投递并排干进行中的 On*，再允许析构
  ~StrategyEventDispatcher();

  StrategyEventDispatcher(const StrategyEventDispatcher&) = delete;
  StrategyEventDispatcher& operator=(const StrategyEventDispatcher&) = delete;

  /// @brief 写入路由快照（按值接管；可在 Subscribe 前后调用以刷新）
  /// @param instrument_routes 品种 → 独占策略
  /// @param strategies 全部策略实例（广播用）
  void SetRouting(std::unordered_map<std::string, qtrade::strategy::IStrategy*> instrument_routes,
                  std::vector<qtrade::strategy::IStrategy*> strategies);

  /// @brief 订阅 Tick/Bar/Order/Trade（可重复调用会叠加订阅，须只调一次）
  void Subscribe();

  /// @brief 开关事件投递；未激活时 On* 直接丢弃
  /// @param active true 开始投递，false 停止投递
  void SetActive(bool active);

 private:
  /// @brief 处理 Tick：有路由则单播，否则广播
  /// @param tick 行情 tick
  void OnTick(const qtrade::sdk::quote::MarketTick& tick);

  /// @brief 处理 Bar：有路由则单播，否则广播
  /// @param bar K线周期柱
  void OnBar(const qtrade::sdk::quote::Bar& bar);

  /// @brief 处理委托回报：有路由则单播，否则广播
  /// @param order 委托
  void OnOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 处理成交回报：有路由则单播，否则广播
  /// @param trade 成交
  void OnTrade(const qtrade::sdk::trader::Trade& trade);

  /// 保护路由快照读写
  mutable std::mutex mutex_;
  /// 是否已 Start；未激活时丢弃事件
  std::atomic<bool> active_ = false;
  /// 是否已完成 Subscribe，防止重复叠加订阅
  bool subscribed_ = false;
  /// 事件通道（Lane-Q / Lane-T）
  events::EventLanes& event_lanes_;
  /// 品种 → 独占策略（自持快照）
  std::unordered_map<std::string, qtrade::strategy::IStrategy*> instrument_routes_;
  /// 全部策略实例列表（自持快照，广播用）
  std::vector<qtrade::strategy::IStrategy*> strategies_;
};

}  // namespace qtrade::engine::strategies

#endif  // QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_
