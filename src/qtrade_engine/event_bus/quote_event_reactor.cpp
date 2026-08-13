/// @file      quote_event_reactor.cpp
/// @brief     Lane-Q 行情 EventReactor 实现
/// @details   负责启停循环、订阅注册、Publish 入队，以及按 EventType 回调
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/event_bus/quote_event_reactor.hpp"

#include <spdlog/spdlog.h>

#include <exception>

namespace qtrade::engine::event_bus {

QuoteEventReactor::QuoteEventReactor() : loop_("QuoteEventReactor") {
  SetLanePolicy(LanePolicy{.drop_oldest_on_full = true});
}

QuoteEventReactor::~QuoteEventReactor() {
  Stop();
}

bool QuoteEventReactor::SetLanePolicy(LanePolicy policy) {
  return loop_.SetLanePolicy(policy);
}

void QuoteEventReactor::Start() {
  // 1. 将循环出队事件交给类型分发器处理
  loop_.Start([this](const EventPtr& event) { HandleEvent(*event); });
}

void QuoteEventReactor::Stop() {
  // 1. 停止消费线程并拒绝新的行情事件
  loop_.Stop();
  // 2. 清理订阅者，解除对策略与外部对象的引用
  std::lock_guard<std::mutex> lock(callbacks_mutex_);
  tick_callbacks_.clear();
  bar_callbacks_.clear();
}

void QuoteEventReactor::SubscribeTick(qtrade::sdk::quote::QuoteApi::TickCallback callback) {
  // 注册 Tick 订阅者，由 HandleEvent 在消费线程中同步回调
  std::lock_guard<std::mutex> lock(callbacks_mutex_);
  tick_callbacks_.push_back(std::move(callback));
}

void QuoteEventReactor::SubscribeBar(qtrade::sdk::quote::QuoteApi::BarCallback callback) {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);
  bar_callbacks_.push_back(std::move(callback));
}

void QuoteEventReactor::PublishTick(const qtrade::sdk::quote::MarketTick& tick) {
  // 1. 复制行情快照，交由 LanePolicy 决定队列满时的处理方式
  loop_.Publish(std::make_unique<TickEvent>(tick));
}

void QuoteEventReactor::PublishBar(const qtrade::sdk::quote::Bar& bar) {
  loop_.Publish(std::make_unique<BarEvent>(bar));
}

bool QuoteEventReactor::HasPending() const {
  return loop_.HasPending();
}

std::size_t QuoteEventReactor::PendingCount() const {
  return loop_.PendingCount();
}

void QuoteEventReactor::HandleEvent(const Event& event) {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);

  // 按 EventType 分发并隔离回调异常
  switch (event.type) {
    /// 行情事件
    case EventType::kTickData: {
      const auto& payload = static_cast<const TickEvent&>(event).tick;
      for (const auto& callback : tick_callbacks_) {
        try {
          callback(payload);
        } catch (const std::exception& e) {
          spdlog::error("[QuoteEventReactor] tick callback exception: {}", e.what());
        }
      }
      break;
    }

    /// 分钟线事件
    case EventType::kBarData: {
      const auto& payload = static_cast<const BarEvent&>(event).bar;
      for (const auto& callback : bar_callbacks_) {
        try {
          callback(payload);
        } catch (const std::exception& e) {
          spdlog::error("[QuoteEventReactor] bar callback exception: {}", e.what());
        }
      }
      break;
    }

    /// 未知事件
    default:
      spdlog::warn("[QuoteEventReactor] ignored event type {}", static_cast<int>(event.type));
      break;
  }
}

}  // namespace qtrade::engine::event_bus
