/// @file      market_event_reactor.cpp
/// @brief     Lane-M 行情 EventReactor 实现
/// @details   负责启停循环、订阅注册、Publish 入队，以及按 EventType 回调 Handler
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/event_bus/market_event_reactor.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

MarketEventReactor::MarketEventReactor() : loop_("MarketEventReactor") {}

MarketEventReactor::~MarketEventReactor() {
  Stop();
}

void MarketEventReactor::Start() {
  loop_.Start([this](const EventPtr& event) { HandleEvent(*event); });
}

void MarketEventReactor::Stop() {
  loop_.Stop();
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  tick_handlers_.clear();
  bar_handlers_.clear();
}

void MarketEventReactor::SubscribeTick(TickEventHandler handler) {
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  tick_handlers_.push_back(std::move(handler));
}

void MarketEventReactor::SubscribeBar(BarEventHandler handler) {
  std::lock_guard<std::mutex> lock(handlers_mutex_);
  bar_handlers_.push_back(std::move(handler));
}

void MarketEventReactor::PublishTick(const qtrade_sdk::quote::MarketTick& tick) {
  loop_.Publish(std::make_unique<TickEvent>(tick));
}

void MarketEventReactor::PublishBar(const qtrade_sdk::quote::Bar& bar) {
  loop_.Publish(std::make_unique<BarEvent>(bar));
}

bool MarketEventReactor::HasPending() const {
  return loop_.HasPending();
}

std::size_t MarketEventReactor::PendingCount() const {
  return loop_.PendingCount();
}

void MarketEventReactor::HandleEvent(const Event& event) {
  // 1. 在锁内快照订阅列表，避免回调期间长时间持锁
  std::vector<TickEventHandler> tick_handlers;
  std::vector<BarEventHandler> bar_handlers;
  {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    tick_handlers = tick_handlers_;
    bar_handlers = bar_handlers_;
  }

  // 2. 按 EventType 分发并隔离 Handler 异常
  switch (event.type) {
    case EventType::kTickData: {
      const auto& payload = static_cast<const TickEvent&>(event).tick;
      for (const auto& handler : tick_handlers) {
        try {
          handler(payload);
        } catch (const std::exception& e) {
          spdlog::error("[MarketEventReactor] tick event handler exception: {}", e.what());
        }
      }
      break;
    }
    case EventType::kBarData: {
      const auto& payload = static_cast<const BarEvent&>(event).bar;
      for (const auto& handler : bar_handlers) {
        try {
          handler(payload);
        } catch (const std::exception& e) {
          spdlog::error("[MarketEventReactor] bar event handler exception: {}", e.what());
        }
      }
      break;
    }
    default:
      spdlog::warn("[MarketEventReactor] ignored event type {}", static_cast<int>(event.type));
      break;
  }
}

}  // namespace qtrade::engine::event_bus
