/// @file      strategy_event_dispatcher.cpp
/// @brief     策略事件分发器实现
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/strategy/strategy_event_dispatcher.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::strategy {

StrategyEventDispatcher::StrategyEventDispatcher(
  event_bus::EventLanes& event_lanes,
  std::mutex& mutex,
  const std::atomic_bool& running,
  const std::unordered_map<std::string, qtrade::strategy::IStrategy*>& instrument_routes,
  const std::vector<qtrade::strategy::IStrategy*>& strategies)
  : event_lanes_(event_lanes),
    mutex_(mutex),
    running_(running),
    instrument_routes_(instrument_routes),
    strategies_(strategies) {}

void StrategyEventDispatcher::Subscribe() {
  if (subscribed_) {
    return;
  }
  event_lanes_.Quote().SubscribeTick([this](const qtrade_sdk::quote::MarketTick& tick) { OnTick(tick); });
  event_lanes_.Quote().SubscribeBar([this](const qtrade_sdk::quote::Bar& bar) { OnBar(bar); });
  event_lanes_.Trader().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) { OnOrder(order); });
  event_lanes_.Trader().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) { OnTrade(trade); });
  subscribed_ = true;
}

void StrategyEventDispatcher::OnTick(const qtrade_sdk::quote::MarketTick& tick) {
  std::lock_guard lock(mutex_);
  if (!running_.load()) {
    return;
  }
  if (const auto route = instrument_routes_.find(tick.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTick exception: {}", e.what());
    }
    return;
  }
  for (auto* strategy : strategies_) {
    try {
      strategy->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTick broadcast exception: {}", e.what());
    }
  }
}

void StrategyEventDispatcher::OnBar(const qtrade_sdk::quote::Bar& bar) {
  std::lock_guard lock(mutex_);
  if (!running_.load()) {
    return;
  }
  if (const auto route = instrument_routes_.find(bar.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnBar exception: {}", e.what());
    }
    return;
  }
  for (auto* strategy : strategies_) {
    try {
      strategy->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnBar broadcast exception: {}", e.what());
    }
  }
}

void StrategyEventDispatcher::OnOrder(const qtrade_sdk::trader::Order& order) {
  std::lock_guard lock(mutex_);
  if (!running_.load()) {
    return;
  }
  if (const auto route = instrument_routes_.find(order.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnOrder(order);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnOrder exception: {}", e.what());
    }
    return;
  }
  for (auto* strategy : strategies_) {
    try {
      strategy->OnOrder(order);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnOrder broadcast exception: {}", e.what());
    }
  }
}

void StrategyEventDispatcher::OnTrade(const qtrade_sdk::trader::Trade& trade) {
  std::lock_guard lock(mutex_);
  if (!running_.load()) {
    return;
  }
  if (const auto route = instrument_routes_.find(trade.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnTrade(trade);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTrade exception: {}", e.what());
    }
    return;
  }
  for (auto* strategy : strategies_) {
    try {
      strategy->OnTrade(trade);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEventDispatcher] OnTrade broadcast exception: {}", e.what());
    }
  }
}

}  // namespace qtrade::engine::strategy
