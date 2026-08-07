/// @file      event_lanes.hpp
/// @brief     EventBus 门面：统一启停 Lane-Q / Lane-T 两条 EventReactor
/// @details   持有 QuoteEventReactor 与 TraderEventReactor，对外提供启停与队列深度查询
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EVENT_LANES_HPP_
#define QTRADE_TRADING_ENGINE_EVENT_LANES_HPP_

#include "qtrade/engine/event_bus/quote_event_reactor.hpp"
#include "qtrade/engine/event_bus/trader_event_reactor.hpp"

#include <cstddef>

namespace qtrade::engine::event_bus {

/// @brief EventBus 子系统入口：持有 QuoteEventReactor + TraderEventReactor
class EventLanes {
 public:
  /// @brief 构造 Lane-Q / Lane-T
  EventLanes() = default;

  /// @brief 启动 Lane-Q 与 Lane-T 两条 EventReactor
  void Start();

  /// @brief 停止两条 EventReactor（先 Return 后 Market）
  void Stop();

  /// @brief 获取可写的行情 EventReactor
  /// @return Lane-Q QuoteEventReactor 引用
  [[nodiscard]] QuoteEventReactor& Quote();

  /// @brief 获取可写的回报 EventReactor
  /// @return Lane-T TraderEventReactor 引用
  [[nodiscard]] TraderEventReactor& Trader();

  /// @brief 获取只读的行情 EventReactor
  /// @return Lane-Q QuoteEventReactor 常量引用
  [[nodiscard]] const QuoteEventReactor& Quote() const;

  /// @brief 获取只读的回报 EventReactor
  /// @return Lane-T TraderEventReactor 常量引用
  [[nodiscard]] const TraderEventReactor& Trader() const;

  /// @brief 查询 Lane-Q 待处理事件数
  /// @return 行情队列当前深度
  [[nodiscard]] std::size_t QuoteQueueSize() const;

  /// @brief 查询 Lane-T 待处理事件数
  /// @return 回报队列当前深度
  [[nodiscard]] std::size_t TraderQueueSize() const;

 private:
  /// Lane-Q 行情 EventReactor
  QuoteEventReactor quote_event_reactor_;
  /// Lane-T 回报 EventReactor
  TraderEventReactor trader_event_reactor_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_EVENT_LANES_HPP_
