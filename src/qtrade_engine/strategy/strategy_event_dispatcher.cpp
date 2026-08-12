/// @file      strategy_event_dispatcher.cpp
/// @brief     策略事件分发器实现
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/strategy/strategy_event_dispatcher.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::engine::strategy {

StrategyEventDispatcher::StrategyEventDispatcher(event_bus::EventLanes& event_lanes) : event_lanes_(event_lanes) {}

StrategyEventDispatcher::~StrategyEventDispatcher() {
  active_.store(false);
  std::lock_guard lock(mutex_);
  instrument_routes_.clear();
  strategies_.clear();
}

void StrategyEventDispatcher::SetRouting(
  std::unordered_map<std::string, qtrade::strategy::IStrategy*> instrument_routes,
  std::vector<qtrade::strategy::IStrategy*> strategies) {
  std::lock_guard lock(mutex_);
  instrument_routes_ = std::move(instrument_routes);
  strategies_ = std::move(strategies);
}

void StrategyEventDispatcher::Subscribe() {
  if (subscribed_) {
    return;
  }
  event_lanes_.Quote().SubscribeTick([this](const qtrade::sdk::quote::MarketTick& tick) { OnTick(tick); });
  event_lanes_.Quote().SubscribeBar([this](const qtrade::sdk::quote::Bar& bar) { OnBar(bar); });
  event_lanes_.Trader().SubscribeOrder([this](const qtrade::sdk::trader::Order& order) { OnOrder(order); });
  event_lanes_.Trader().SubscribeTrade([this](const qtrade::sdk::trader::Trade& trade) { OnTrade(trade); });
  subscribed_ = true;
}

void StrategyEventDispatcher::SetActive(bool active) {
  active_.store(active);
}

void StrategyEventDispatcher::OnTick(const qtrade::sdk::quote::MarketTick& tick) {
  std::lock_guard lock(mutex_);
  if (!active_.load()) {
    return;
  }

  // 1. 有品种路由则单播
  if (const auto route = instrument_routes_.find(tick.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTick exception: {}", e.what());
    }
    return;
  }

  // 2. 否则广播给全部策略
  for (auto* strategy : strategies_) {
    try {
      strategy->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTick broadcast exception: {}", e.what());
    }
  }
}

void StrategyEventDispatcher::OnBar(const qtrade::sdk::quote::Bar& bar) {
  std::lock_guard lock(mutex_);
  if (!active_.load()) {
    return;
  }

  // 1. 有品种路由则单播
  if (const auto route = instrument_routes_.find(bar.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnBar exception: {}", e.what());
    }
    return;
  }

  // 2. 否则广播给全部策略
  for (auto* strategy : strategies_) {
    try {
      strategy->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnBar broadcast exception: {}", e.what());
    }
  }
}

void StrategyEventDispatcher::OnOrder(const qtrade::sdk::trader::Order& order) {
  std::lock_guard lock(mutex_);
  if (!active_.load()) {
    return;
  }

  // 1. 有品种路由则单播
  if (const auto route = instrument_routes_.find(order.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnOrder(order);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnOrder exception: {}", e.what());
    }
    return;
  }

  // 2. 否则广播给全部策略
  for (auto* strategy : strategies_) {
    try {
      strategy->OnOrder(order);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnOrder broadcast exception: {}", e.what());
    }
  }
}

void StrategyEventDispatcher::OnTrade(const qtrade::sdk::trader::Trade& trade) {
  std::lock_guard lock(mutex_);
  if (!active_.load()) {
    return;
  }

  // 1. 有品种路由则单播
  if (const auto route = instrument_routes_.find(trade.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnTrade(trade);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTrade exception: {}", e.what());
    }
    return;
  }

  // 2. 否则广播给全部策略
  for (auto* strategy : strategies_) {
    try {
      strategy->OnTrade(trade);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTrade broadcast exception: {}", e.what());
    }
  }
}

}  // namespace qtrade::engine::strategy
