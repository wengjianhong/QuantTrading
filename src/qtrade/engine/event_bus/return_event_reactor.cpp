/// @file      return_event_reactor.cpp
/// @brief     Lane-R 回报 EventReactor 实现
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/event_bus/return_event_reactor.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

ReturnEventReactor::ReturnEventReactor() : loop_("ReturnEventReactor") {}

ReturnEventReactor::~ReturnEventReactor() {
  Stop();
}

void ReturnEventReactor::Start() {
  loop_.Start([this](const EventPtr& event) { HandleEvent(*event); });
}

void ReturnEventReactor::Stop() {
  loop_.Stop();
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  order_handlers_.clear();
  trade_handlers_.clear();
}

void ReturnEventReactor::SubscribeOrder(OrderEventHandler handler) {
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  order_handlers_.push_back(std::move(handler));
}

void ReturnEventReactor::SubscribeTrade(TradeEventHandler handler) {
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  trade_handlers_.push_back(std::move(handler));
}

void ReturnEventReactor::PublishOrder(const qtrade_sdk::trader::Order& order) {
  loop_.Publish(std::make_unique<OrderEvent>(order));
}

void ReturnEventReactor::PublishTrade(const qtrade_sdk::trader::Trade& trade) {
  loop_.Publish(std::make_unique<TradeEvent>(trade));
}

bool ReturnEventReactor::HasPending() const {
  return loop_.HasPending();
}

std::size_t ReturnEventReactor::PendingCount() const {
  return loop_.PendingCount();
}

void ReturnEventReactor::HandleEvent(const Event& event) {
  std::vector<OrderEventHandler> order_handlers;
  std::vector<TradeEventHandler> trade_handlers;
  {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    order_handlers = order_handlers_;
    trade_handlers = trade_handlers_;
  }

  switch (event.type) {
    case EventType::kOrderUpdate: {
      const auto& payload = static_cast<const OrderEvent&>(event).order;
      for (const auto& handler : order_handlers) {
        try {
          handler(payload);
        } catch (const std::exception& e) {
          spdlog::error("[ReturnEventReactor] order event handler exception: {}", e.what());
        }
      }
      break;
    }
    case EventType::kTradeUpdate: {
      const auto& payload = static_cast<const TradeEvent&>(event).trade;
      for (const auto& handler : trade_handlers) {
        try {
          handler(payload);
        } catch (const std::exception& e) {
          spdlog::error("[ReturnEventReactor] trade event handler exception: {}", e.what());
        }
      }
      break;
    }
    default:
      spdlog::warn("[ReturnEventReactor] ignored event type {}", static_cast<int>(event.type));
      break;
  }
}

}  // namespace qtrade::engine::event_bus
