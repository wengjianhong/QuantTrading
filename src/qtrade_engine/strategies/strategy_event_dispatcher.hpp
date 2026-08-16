/// @file      strategy_event_dispatcher.hpp
/// @brief     策略事件分发器：订阅 EventLanes 并按品种路由到策略事件队列
/// @details   路由表与广播列表在 SetRouting 时拷贝自持；Lane 回调只入队，不调用策略 On*。
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_
#define QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_

#include "qtrade/engine/events/event_lanes.hpp"
#include "qtrade/engine/strategies/strategy_event_queue.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::strategies {

/// @brief 按 instrument 将行情/回报入队到策略队列；无路由时广播
class StrategyEventDispatcher {
 public:
  /// @brief 仅绑定事件通道
  /// @param event_lanes Lane-Q / Lane-T
  explicit StrategyEventDispatcher(events::EventLanes& event_lanes);

  /// @brief 停投递后再允许析构
  ~StrategyEventDispatcher();

  /// @brief 禁止移动构造
  StrategyEventDispatcher(StrategyEventDispatcher&&) = delete;
  /// @brief 禁止拷贝构造
  StrategyEventDispatcher(const StrategyEventDispatcher&) = delete;
  /// @brief 禁止移动赋值
  StrategyEventDispatcher& operator=(StrategyEventDispatcher&&) = delete;
  /// @brief 禁止拷贝赋值
  StrategyEventDispatcher& operator=(const StrategyEventDispatcher&) = delete;

  /// @brief 写入路由快照（按值接管；可在 Subscribe 前后调用以刷新）
  /// @param instrument_routes 品种 → 独占策略队列
  /// @param queues 全部策略队列（广播用）
  void SetRouting(std::unordered_map<std::string, StrategyEventQueue*> instrument_routes,
                  std::vector<StrategyEventQueue*> queues);

  /// @brief 订阅 Tick/Bar/Order/Trade（可重复调用会叠加订阅，须只调一次）
  void Subscribe();

  /// @brief 开关事件投递；未激活时 On* 直接丢弃
  /// @param active true 开始投递，false 停止投递
  void SetActive(bool active);

 private:
  /// @brief 处理 Tick：有路由则单播入队，否则广播
  /// @param tick 行情 tick
  void OnTick(const qtrade::sdk::quote::MarketTick& tick);

  /// @brief 处理 Bar：有路由则单播入队，否则广播
  /// @param bar K线周期柱
  void OnBar(const qtrade::sdk::quote::Bar& bar);

  /// @brief 处理委托回报：有路由则单播入队，否则广播
  /// @param order 委托
  void OnOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 处理成交回报：有路由则单播入队，否则广播
  /// @param trade 成交
  void OnTrade(const qtrade::sdk::trader::Trade& trade);

  /// @brief 按品种解析目标队列；无路由时返回广播列表
  /// @param instrument 合约代码
  /// @return 目标队列指针列表（须在持锁时调用）
  [[nodiscard]] std::vector<StrategyEventQueue*> ResolveTargetsLocked(const std::string& instrument) const;

  /// 保护路由快照读写
  mutable std::mutex mutex_;
  /// 是否已 Start；未激活时丢弃事件
  std::atomic<bool> active_ = false;
  /// 是否已完成 Subscribe，防止重复叠加订阅
  bool subscribed_ = false;
  /// 事件通道（Lane-Q / Lane-T）
  events::EventLanes& event_lanes_;
  /// 品种 → 独占策略队列（自持快照）
  std::unordered_map<std::string, StrategyEventQueue*> instrument_routes_;
  /// 全部策略队列（自持快照，广播用）
  std::vector<StrategyEventQueue*> queues_;
};

}  // namespace qtrade::engine::strategies

#endif  // QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_
