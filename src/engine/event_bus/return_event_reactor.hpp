/// @file      return_event_reactor.hpp
/// @brief     Lane-R 回报 EventReactor（EventBus 子系统实现）
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_RETURN_EVENT_REACTOR_HPP_
#define QTRADE_TRADING_ENGINE_RETURN_EVENT_REACTOR_HPP_

#include "engine/event_bus/event_reactor_loop.hpp"
#include "engine/event_bus/event_types.hpp"

#include <cstddef>
#include <mutex>
#include <vector>

namespace qtrade::engine::event_bus {

/// @brief Lane-R EventReactor：`EventPtr` FIFO 入队，按 `EventType` 调用 EventHandler
class ReturnEventReactor {
 public:
  ReturnEventReactor();
  ~ReturnEventReactor();

  ReturnEventReactor(const ReturnEventReactor&) = delete;
  ReturnEventReactor& operator=(const ReturnEventReactor&) = delete;

  void Start();
  void Stop();

  void SubscribeOrder(OrderEventHandler handler);
  void SubscribeTrade(TradeEventHandler handler);

  void PublishOrder(const qtrade_sdk::trader::Order& order);
  void PublishTrade(const qtrade_sdk::trader::Trade& trade);

  [[nodiscard]] bool HasPending() const;
  [[nodiscard]] std::size_t PendingCount() const;

 private:
  void HandleEvent(const Event& event);

  EventReactorLoop<EventPtr, ReturnLanePolicy> loop_;
  std::vector<OrderEventHandler> order_handlers_;
  std::vector<TradeEventHandler> trade_handlers_;
  mutable std::mutex handlers_mutex_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_RETURN_EVENT_REACTOR_HPP_
