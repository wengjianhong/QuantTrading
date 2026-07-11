/// @file      event_lanes.hpp
/// @brief     EventBus 门面：统一启停 Lane-M / Lane-R 两条 EventReactor
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
  void Start();
  void Stop();

  [[nodiscard]] MarketEventReactor& Market() {
    return market_event_reactor_;
  }
  [[nodiscard]] ReturnEventReactor& Return() {
    return return_event_reactor_;
  }

  [[nodiscard]] const MarketEventReactor& Market() const {
    return market_event_reactor_;
  }
  [[nodiscard]] const ReturnEventReactor& Return() const {
    return return_event_reactor_;
  }

  [[nodiscard]] std::size_t MarketQueueSize() const {
    return market_event_reactor_.PendingCount();
  }
  [[nodiscard]] std::size_t ReturnQueueSize() const {
    return return_event_reactor_.PendingCount();
  }

 private:
  MarketEventReactor market_event_reactor_;
  ReturnEventReactor return_event_reactor_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_EVENT_LANES_HPP_
