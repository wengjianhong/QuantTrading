/// @file      strategy_engine.cpp
/// @brief     策略引擎实现
/// @details   实现策略注册、生命周期管理及市场数据分发
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "strategy_engine.hpp"
#include <spdlog/spdlog.h>

namespace qtrade::engine::strategy {

StrategyEngine::StrategyEngine(event_bus::EventLanes& event_lanes) : event_lanes_(event_lanes), running_(false) {}

StrategyEngine::~StrategyEngine() { Stop(); }

void StrategyEngine::Start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) {
    return;
  }
  running_ = true;

  // 订阅事件总线
  event_lanes_.Market().SubscribeTick([this](const qtrade_sdk::quote::MarketTick& tick) { OnTickEvent(tick); });
  event_lanes_.Market().SubscribeBar([this](const qtrade_sdk::quote::Bar& bar) { OnBarEvent(bar); });
  event_lanes_.Return().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) { OnOrderEvent(order); });
  event_lanes_.Return().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) { OnTradeEvent(trade); });

  // 启动所有策略
  for (auto& strategy : strategies_) {
    strategy->Start();
  }

  spdlog::info("[StrategyEngine] started successfully with {} strategies", strategies_.size());
}

void StrategyEngine::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return;
  }

  // 停止所有策略
  for (auto& strategy : strategies_) {
    strategy->Stop();
  }

  running_ = false;
  spdlog::info("[StrategyEngine] stopped cleanly");
}

void StrategyEngine::RegisterStrategy(std::unique_ptr<qtrade::strategy::IStrategy> strategy) {
  std::lock_guard<std::mutex> lock(mutex_);
  strategies_.push_back(std::move(strategy));
  spdlog::info("[StrategyEngine] registered new strategy");
}

void StrategyEngine::SetOrderSender(OrderSender sender) {
  std::lock_guard<std::mutex> lock(mutex_);
  order_sender_ = std::move(sender);
}

void StrategyEngine::OnTickEvent(const qtrade_sdk::quote::MarketTick& tick) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& strategy : strategies_) {
    try {
      strategy->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnTick exception: {}", e.what());
    }
  }
}

void StrategyEngine::OnBarEvent(const qtrade_sdk::quote::Bar& bar) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& strategy : strategies_) {
    try {
      strategy->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnBar exception: {}", e.what());
    }
  }
}

void StrategyEngine::OnOrderEvent(const qtrade_sdk::trader::Order& order) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& strategy : strategies_) {
    try {
      strategy->OnOrder(order);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnOrder exception: {}", e.what());
    }
  }
}

void StrategyEngine::OnTradeEvent(const qtrade_sdk::trader::Trade& trade) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& strategy : strategies_) {
    try {
      strategy->OnTrade(trade);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnTrade exception: {}", e.what());
    }
  }
}

}  // namespace qtrade::engine::strategy
