/// @file      strategy_event_dispatcher.hpp
/// @brief     策略事件分发器：订阅 EventLanes 并按品种路由到策略实例
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_
#define QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/strategy/strategy.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::strategy {

/// @brief 按 instrument 路由行情/回报到策略；无路由时广播
class StrategyEventDispatcher {
 public:
  /// @brief 绑定事件通道与路由表（表由 StrategyManager 持有）
  /// @param event_lanes Lane-Q / Lane-T
  /// @param mutex 与 Manager 共用的调度锁
  /// @param running 是否已 Start
  /// @param instrument_routes 品种 → 策略
  /// @param strategies 全部策略实例（广播用）
  StrategyEventDispatcher(event_bus::EventLanes& event_lanes,
                          std::mutex& mutex,
                          const std::atomic_bool& running,
                          const std::unordered_map<std::string, qtrade::strategy::IStrategy*>& instrument_routes,
                          const std::vector<qtrade::strategy::IStrategy*>& strategies);

  /// @brief 订阅 Tick/Bar/Order/Trade（可重复调用会叠加订阅，须只调一次）
  void Subscribe();

 private:
  void OnTick(const qtrade_sdk::quote::MarketTick& tick);
  void OnBar(const qtrade_sdk::quote::Bar& bar);
  void OnOrder(const qtrade_sdk::trader::Order& order);
  void OnTrade(const qtrade_sdk::trader::Trade& trade);

  event_bus::EventLanes& event_lanes_;
  std::mutex& mutex_;
  const std::atomic_bool& running_;
  const std::unordered_map<std::string, qtrade::strategy::IStrategy*>& instrument_routes_;
  const std::vector<qtrade::strategy::IStrategy*>& strategies_;
  bool subscribed_ = false;
};

}  // namespace qtrade::engine::strategy

#endif  // QTRADE_ENGINE_STRATEGY_EVENT_DISPATCHER_HPP_
