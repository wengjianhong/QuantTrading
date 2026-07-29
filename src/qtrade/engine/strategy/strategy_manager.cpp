/// @file      strategy_manager.cpp
/// @brief     策略管理器实现
/// @details   Init 阶段注册实例；Start/Stop 与引擎生命周期对齐；Stop 后清空须重新装配
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "strategy_manager.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::strategy {

StrategyManager::StrategyManager(event_bus::EventLanes& event_lanes) : event_lanes_(event_lanes), running_(false) {}

StrategyManager::~StrategyManager() {
  Stop();
}

void StrategyManager::Start() {
  std::lock_guard lock(mutex_);
  if (running_) {
    return;
  }
  running_ = true;

  event_lanes_.Quote().SubscribeTick([this](const qtrade_sdk::quote::MarketTick& tick) { OnTickEvent(tick); });
  event_lanes_.Quote().SubscribeBar([this](const qtrade_sdk::quote::Bar& bar) { OnBarEvent(bar); });
  event_lanes_.Trader().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) { OnOrderEvent(order); });
  event_lanes_.Trader().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) { OnTradeEvent(trade); });

  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (entry.strategy->Start() != ErrorCode::kSuccess) {
      spdlog::error("[StrategyManager] strategy Start failed");
    }
  }

  spdlog::info("[StrategyManager] started with {} strategies", strategies_.size());
}

void StrategyManager::Stop() {
  std::lock_guard lock(mutex_);
  if (running_) {
    for (auto& [strategy_id, entry] : strategies_) {
      (void)strategy_id;
      entry.strategy->Stop();
    }
    running_ = false;
  }
  // Stop 后清空，强制下次经 Init/LoadStrategies 重新装配
  strategies_.clear();
  instrument_routes_.clear();
  spdlog::info("[StrategyManager] stopped and cleared");
}

ErrorCode StrategyManager::RegisterStrategy(const std::string& strategy_id,
                                            StrategyPtr strategy,
                                            const std::vector<std::string>& instruments) {
  if (strategy_id.empty() || !strategy) {
    return ErrorCode::kInternalError;
  }
  std::lock_guard lock(mutex_);
  if (running_) {
    return ErrorCode::kSystemError;
  }
  if (strategies_.contains(strategy_id)) {
    return ErrorCode::kSystemError;
  }
  for (const auto& instrument : instruments) {
    if (instrument.empty() || instrument_routes_.contains(instrument)) {
      return ErrorCode::kSystemError;
    }
  }
  auto* raw = strategy.get();
  for (const auto& instrument : instruments) {
    instrument_routes_[instrument] = raw;
  }
  strategies_.emplace(strategy_id, StrategyEntry{std::move(strategy), instruments});
  return ErrorCode::kSuccess;
}

void StrategyManager::OnTickEvent(const qtrade_sdk::quote::MarketTick& tick) {
  std::lock_guard lock(mutex_);
  if (!running_) {
    return;
  }
  if (const auto route = instrument_routes_.find(tick.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyManager] routed strategy OnTick exception: {}", e.what());
    }
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    try {
      entry.strategy->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyManager] strategy OnTick exception: {}", e.what());
    }
  }
}

void StrategyManager::OnBarEvent(const qtrade_sdk::quote::Bar& bar) {
  std::lock_guard lock(mutex_);
  if (!running_) {
    return;
  }
  if (const auto route = instrument_routes_.find(bar.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyManager] routed strategy OnBar exception: {}", e.what());
    }
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    try {
      entry.strategy->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyManager] strategy OnBar exception: {}", e.what());
    }
  }
}

void StrategyManager::OnOrderEvent(const qtrade_sdk::trader::Order& order) {
  std::lock_guard lock(mutex_);
  if (!running_) {
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    try {
      entry.strategy->OnOrder(order);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyManager] strategy OnOrder exception: {}", e.what());
    }
  }
}

void StrategyManager::OnTradeEvent(const qtrade_sdk::trader::Trade& trade) {
  std::lock_guard lock(mutex_);
  if (!running_) {
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    try {
      entry.strategy->OnTrade(trade);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyManager] strategy OnTrade exception: {}", e.what());
    }
  }
}

}  // namespace qtrade::engine::strategy
