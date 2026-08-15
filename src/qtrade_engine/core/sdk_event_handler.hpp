/// @file      sdk_event_handler.hpp
/// @brief     SDK 回调入站：校验后写入 Lane-Q / Lane-T
/// @details   在适配器线程执行；不是 Lane 出站消费者（见 LaneEventHandler）。
/// @author    wengjianhong
/// @date      2026-08-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_SDK_EVENT_HANDLER_HPP_
#define QTRADE_ENGINE_SDK_EVENT_HANDLER_HPP_

#include "qtrade/engine/core/quote_health_monitor.hpp"
#include "qtrade/engine/events/event_lanes.hpp"

#include <qtrade/sdk/quote/quote_api.hpp>
#include <qtrade/sdk/trader/trader_api.hpp>

#include <atomic>

namespace qtrade::engine {

/// @brief QuoteApi / TraderApi 回调入口（行情与回报共用）
class SdkEventHandler {
 public:
  /// @brief 绑定运行门禁、事件通道与行情健康监控（不拥有）
  /// @param running 引擎是否已 Start
  /// @param event_lanes Lane-Q / Lane-T
  /// @param quote_health 有效/无效 Tick 健康计数
  SdkEventHandler(std::atomic<bool>& running, events::EventLanes& event_lanes, QuoteHealthMonitor& quote_health);

  /// @brief Tick 入站：运行门禁、校验、健康计数后写入 Lane-Q
  /// @param tick 行情 Tick
  void OnTick(const qtrade::sdk::quote::MarketTick& tick);

  /// @brief Bar 入站：运行门禁、校验后写入 Lane-Q
  /// @param bar 行情 Bar
  void OnBar(const qtrade::sdk::quote::Bar& bar);

  /// @brief 订单回报入站：运行门禁、校验后写入 Lane-T
  /// @param order 订单回报
  void OnOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 成交回报入站：运行门禁、校验后写入 Lane-T
  /// @param trade 成交回报
  void OnTrade(const qtrade::sdk::trader::Trade& trade);

 private:
  /// 引擎 running_（非拥有）
  std::atomic<bool>& running_;
  /// 事件通道（非拥有）
  events::EventLanes& event_lanes_;
  /// 行情健康监控（非拥有）
  QuoteHealthMonitor& quote_health_monitor_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_SDK_EVENT_HANDLER_HPP_
