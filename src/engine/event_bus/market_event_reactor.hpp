/// @file      market_event_reactor.hpp
/// @brief     Lane-M 行情 EventReactor（EventBus 子系统实现）
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_MARKET_EVENT_REACTOR_HPP_
#define QTRADE_TRADING_ENGINE_MARKET_EVENT_REACTOR_HPP_

#include "engine/event_bus/event_reactor_loop.hpp"
#include "engine/event_bus/event_types.hpp"

#include <cstddef>
#include <mutex>
#include <vector>

namespace qtrade::engine::event_bus {

/// @brief Lane-M EventReactor：`EventPtr` FIFO 入队，按 `EventType` 调用 EventHandler
class MarketEventReactor {
 public:
  MarketEventReactor();
  ~MarketEventReactor();

  MarketEventReactor(const MarketEventReactor&) = delete;
  MarketEventReactor& operator=(const MarketEventReactor&) = delete;

  void Start();
  void Stop();

  void SubscribeTick(TickEventHandler handler);
  void SubscribeBar(BarEventHandler handler);

  void PublishTick(const qtrade_sdk::quote::MarketTick& tick);
  void PublishBar(const qtrade_sdk::quote::Bar& bar);

  [[nodiscard]] bool HasPending() const;
  [[nodiscard]] std::size_t PendingCount() const;

 private:
  void HandleEvent(const Event& event);

  EventReactorLoop<EventPtr, MarketLanePolicy> loop_;
  std::vector<TickEventHandler> tick_handlers_;
  std::vector<BarEventHandler> bar_handlers_;
  mutable std::mutex handlers_mutex_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_MARKET_EVENT_REACTOR_HPP_
