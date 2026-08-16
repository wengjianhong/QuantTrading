/// @file      strategy_event_dispatcher.cpp
/// @brief     策略事件分发器实现
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/strategies/strategy_event_dispatcher.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::engine::strategies {

StrategyEventDispatcher::StrategyEventDispatcher(events::EventLanes& event_lanes) : event_lanes_(event_lanes) {}

StrategyEventDispatcher::~StrategyEventDispatcher() {
  active_.store(false);
  std::lock_guard lock(mutex_);
  instrument_routes_.clear();
  queues_.clear();
}

void StrategyEventDispatcher::SetRouting(std::unordered_map<std::string, StrategyEventQueue*> instrument_routes,
                                         std::vector<StrategyEventQueue*> queues) {
  std::lock_guard lock(mutex_);
  instrument_routes_ = std::move(instrument_routes);
  queues_ = std::move(queues);
}

void StrategyEventDispatcher::Subscribe() {
  if (subscribed_) {
    return;
  }
  event_lanes_.Quote().RegisterTickCallback([this](const qtrade::sdk::quote::MarketTick& tick) { OnTick(tick); });
  event_lanes_.Quote().RegisterBarCallback([this](const qtrade::sdk::quote::Bar& bar) { OnBar(bar); });
  event_lanes_.Trader().RegisterOrderCallback([this](const qtrade::sdk::trader::Order& order) { OnOrder(order); });
  event_lanes_.Trader().RegisterTradeCallback([this](const qtrade::sdk::trader::Trade& trade) { OnTrade(trade); });
  subscribed_ = true;
}

void StrategyEventDispatcher::SetActive(bool active) {
  active_.store(active);
}

std::vector<StrategyEventQueue*> StrategyEventDispatcher::ResolveTargetsLocked(const std::string& instrument) const {
  if (const auto route = instrument_routes_.find(instrument); route != instrument_routes_.end()) {
    return {route->second};
  }
  return queues_;
}

void StrategyEventDispatcher::OnTick(const qtrade::sdk::quote::MarketTick& tick) {
  std::vector<StrategyEventQueue*> targets;
  {
    std::lock_guard lock(mutex_);
    if (!active_.load()) {
      return;
    }
    targets = ResolveTargetsLocked(tick.instrument);
  }
  for (auto* queue : targets) {
    if (queue != nullptr && !queue->EnqueueTick(tick)) {
      spdlog::warn("[StrategyEventDispatcher] EnqueueTick rejected instrument={}", tick.instrument);
    }
  }
}

void StrategyEventDispatcher::OnBar(const qtrade::sdk::quote::Bar& bar) {
  std::vector<StrategyEventQueue*> targets;
  {
    std::lock_guard lock(mutex_);
    if (!active_.load()) {
      return;
    }
    targets = ResolveTargetsLocked(bar.instrument);
  }
  for (auto* queue : targets) {
    if (queue != nullptr && !queue->EnqueueBar(bar)) {
      spdlog::warn("[StrategyEventDispatcher] EnqueueBar rejected instrument={}", bar.instrument);
    }
  }
}

void StrategyEventDispatcher::OnOrder(const qtrade::sdk::trader::Order& order) {
  std::vector<StrategyEventQueue*> targets;
  {
    std::lock_guard lock(mutex_);
    if (!active_.load()) {
      return;
    }
    targets = ResolveTargetsLocked(order.instrument);
  }
  for (auto* queue : targets) {
    if (queue != nullptr && !queue->EnqueueOrder(order)) {
      spdlog::warn("[StrategyEventDispatcher] EnqueueOrder rejected instrument={}", order.instrument);
    }
  }
}

void StrategyEventDispatcher::OnTrade(const qtrade::sdk::trader::Trade& trade) {
  std::vector<StrategyEventQueue*> targets;
  {
    std::lock_guard lock(mutex_);
    if (!active_.load()) {
      return;
    }
    targets = ResolveTargetsLocked(trade.instrument);
  }
  for (auto* queue : targets) {
    if (queue != nullptr && !queue->EnqueueTrade(trade)) {
      spdlog::warn("[StrategyEventDispatcher] EnqueueTrade rejected instrument={}", trade.instrument);
    }
  }
}

}  // namespace qtrade::engine::strategies
