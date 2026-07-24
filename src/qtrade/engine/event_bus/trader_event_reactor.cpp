/// @file      trader_event_reactor.cpp
/// @brief     Lane-R 回报 EventReactor 实现
/// @details   负责启停循环、订阅注册、Publish 入队，以及按 EventType 回调 Handler
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/event_bus/trader_event_reactor.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

TraderEventReactor::TraderEventReactor() : loop_("TraderEventReactor") {}

TraderEventReactor::~TraderEventReactor() {
  Stop();
}

void TraderEventReactor::Start() {
  loop_.Start([this](const EventPtr& event) { HandleEvent(*event); });
}

void TraderEventReactor::Stop() {
  loop_.Stop();
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  order_handlers_.clear();
  trade_handlers_.clear();
}

void TraderEventReactor::SubscribeOrder(OrderEventHandler handler) {
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  order_handlers_.push_back(std::move(handler));
}

void TraderEventReactor::SubscribeTrade(TradeEventHandler handler) {
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  trade_handlers_.push_back(std::move(handler));
}

void TraderEventReactor::PublishOrder(const qtrade_sdk::trader::Order& order) {
  loop_.Publish(std::make_unique<OrderEvent>(order));
}

void TraderEventReactor::PublishTrade(const qtrade_sdk::trader::Trade& trade) {
  loop_.Publish(std::make_unique<TradeEvent>(trade));
}

bool TraderEventReactor::HasPending() const {
  return loop_.HasPending();
}

std::size_t TraderEventReactor::PendingCount() const {
  return loop_.PendingCount();
}

void TraderEventReactor::HandleEvent(const Event& event) {
  // 1. 在锁内快照订阅列表，避免回调期间长时间持锁
  std::vector<OrderEventHandler> order_handlers;
  std::vector<TradeEventHandler> trade_handlers;
  {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    order_handlers = order_handlers_;
    trade_handlers = trade_handlers_;
  }

  // 2. 按 EventType 分发并隔离 Handler 异常
  switch (event.type) {
    case EventType::kOrderUpdate: {
      const auto& payload = static_cast<const OrderEvent&>(event).order;
      for (const auto& handler : order_handlers) {
        try {
          handler(payload);
        } catch (const std::exception& e) {
          spdlog::error("[TraderEventReactor] order event handler exception: {}", e.what());
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
          spdlog::error("[TraderEventReactor] trade event handler exception: {}", e.what());
        }
      }
      break;
    }
    default:
      spdlog::warn("[TraderEventReactor] ignored event type {}", static_cast<int>(event.type));
      break;
  }
}

}  // namespace qtrade::engine::event_bus
