/// @file      event_lanes.hpp
/// @brief     EventBus 门面：统一启停 Lane-M / Lane-R 两条 EventReactor
/// @details   持有 MarketEventReactor 与 ReturnEventReactor，对外提供启停与队列深度查询
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EVENT_LANES_HPP_
#define QTRADE_TRADING_ENGINE_EVENT_LANES_HPP_

#include "qtrade/engine/event_bus/market_event_reactor.hpp"
#include "qtrade/engine/event_bus/return_event_reactor.hpp"

#include <cstddef>

namespace qtrade::engine::event_bus {

/// @brief EventBus 子系统入口：持有 MarketEventReactor + ReturnEventReactor
class EventLanes {
 public:
  /// @brief 启动 Lane-M 与 Lane-R 两条 EventReactor
  void Start();

  /// @brief 停止两条 EventReactor（先 Return 后 Market）
  void Stop();

  /// @brief 获取可写的行情 EventReactor
  /// @return Lane-M MarketEventReactor 引用
  [[nodiscard]] MarketEventReactor& Market();

  /// @brief 获取可写的回报 EventReactor
  /// @return Lane-R ReturnEventReactor 引用
  [[nodiscard]] ReturnEventReactor& Return();

  /// @brief 获取只读的行情 EventReactor
  /// @return Lane-M MarketEventReactor 常量引用
  [[nodiscard]] const MarketEventReactor& Market() const;

  /// @brief 获取只读的回报 EventReactor
  /// @return Lane-R ReturnEventReactor 常量引用
  [[nodiscard]] const ReturnEventReactor& Return() const;

  /// @brief 查询 Lane-M 待处理事件数
  /// @return 行情队列当前深度
  [[nodiscard]] std::size_t MarketQueueSize() const;

  /// @brief 查询 Lane-R 待处理事件数
  /// @return 回报队列当前深度
  [[nodiscard]] std::size_t ReturnQueueSize() const;

 private:
  /// Lane-M 行情 EventReactor
  MarketEventReactor market_event_reactor_;
  /// Lane-R 回报 EventReactor
  ReturnEventReactor return_event_reactor_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_EVENT_LANES_HPP_
