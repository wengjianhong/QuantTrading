/// @file      trader_event_reactor.cpp
/// @brief     Lane-T 回报 EventReactor 实现
/// @details   负责启停循环、订阅注册、Publish 入队，以及按 EventType 回调 Handler
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/event_bus/trader_event_reactor.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

TraderEventReactor::TraderEventReactor(LanePolicy policy) : loop_("TraderEventReactor", policy) {}

TraderEventReactor::~TraderEventReactor() {
  Stop();
}

void TraderEventReactor::Start() {
  // 1. 将订单与成交回报交给类型分发器处理
  loop_.Start([this](const EventPtr& event) { HandleEvent(*event); });
}

void TraderEventReactor::Stop() {
  // 1. 停止消费线程并拒绝新的交易回报
  loop_.Stop();

  // 2. 清理订阅者，解除对订单状态协调逻辑的引用
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
  // 1. 复制订单回报；队列满时由 LanePolicy 决定是否拒绝
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
  std::lock_guard<std::mutex> lock(handlers_mutex_);

  // 按 EventType 分发并隔离 Handler 异常
  switch (event.type) {
    /// 订单回报事件
    case EventType::kOrderUpdate: {
      const auto& payload = static_cast<const OrderEvent&>(event).order;
      for (const auto& handler : order_handlers_) {
        try {
          handler(payload);
        } catch (const std::exception& e) {
          spdlog::error("[TraderEventReactor] order event handler exception: {}", e.what());
        }
      }
      break;
    }

    /// 成交回报事件
    case EventType::kTradeUpdate: {
      const auto& payload = static_cast<const TradeEvent&>(event).trade;
      for (const auto& handler : trade_handlers_) {
        try {
          handler(payload);
        } catch (const std::exception& e) {
          spdlog::error("[TraderEventReactor] trade event handler exception: {}", e.what());
        }
      }
      break;
    }

    /// 未知事件
    default:
      spdlog::warn("[TraderEventReactor] ignored event type {}", static_cast<int>(event.type));
      break;
  }
}

}  // namespace qtrade::engine::event_bus
