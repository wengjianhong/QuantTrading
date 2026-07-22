/// @file      strategy_engine.cpp
/// @brief     策略引擎实现
/// @details   实现策略注册、生命周期管理及市场数据分发
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "strategy_engine.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <unordered_set>

namespace qtrade::engine::strategy {

StrategyEngine::StrategyEngine(event_bus::EventLanes& event_lanes) : event_lanes_(event_lanes), running_(false) {}

StrategyEngine::~StrategyEngine() {
  Stop();
}

void StrategyEngine::Start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) {
    return;
  }
  running_ = true;

  // 1. 订阅 Lane-M / Lane-R
  event_lanes_.Market().SubscribeTick([this](const qtrade_sdk::quote::MarketTick& tick) { OnTickEvent(tick); });
  event_lanes_.Market().SubscribeBar([this](const qtrade_sdk::quote::Bar& bar) { OnBarEvent(bar); });
  event_lanes_.Return().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) { OnOrderEvent(order); });
  event_lanes_.Return().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) { OnTradeEvent(trade); });

  // 2. 启动已启用且未启动的策略
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (entry.enabled && !entry.started && entry.strategy->Start() == ErrorCode::kSuccess) {
      entry.started = true;
    }
  }

  spdlog::info("[StrategyEngine] started successfully with {} strategies", strategies_.size());
}

void StrategyEngine::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return;
  }

  // 停止已启动的策略
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (entry.started) {
      entry.strategy->Stop();
      entry.started = false;
    }
  }

  running_ = false;
  spdlog::info("[StrategyEngine] stopped cleanly");
}

void StrategyEngine::RegisterStrategy(std::unique_ptr<qtrade::strategy::IStrategy> strategy) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!strategy) {
    return;
  }
  const std::string strategy_id = "manual-" + std::to_string(++manual_strategy_counter_);
  strategies_.emplace(strategy_id, StrategyEntry{std::move(strategy), true, false, false, {}});
  spdlog::info("[StrategyEngine] registered new strategy");
}

ErrorCode StrategyEngine::RegisterStrategy(std::unique_ptr<qtrade::strategy::IStrategy> strategy,
                                           const std::vector<std::string>& instruments) {
  if (!strategy || instruments.empty()) {
    return ErrorCode::kInternalError;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& instrument : instruments) {
    if (instrument.empty() || instrument_routes_.contains(instrument)) {
      return ErrorCode::kSystemError;
    }
  }
  auto* raw = strategy.get();
  const std::string strategy_id = "manual-" + std::to_string(++manual_strategy_counter_);
  for (const auto& instrument : instruments) {
    instrument_routes_.emplace(instrument, raw);
  }
  strategies_.emplace(strategy_id, StrategyEntry{std::move(strategy), true, false, false, instruments});
  return ErrorCode::kSuccess;
}

ErrorCode StrategyEngine::RegisterStrategy(const std::string& strategy_id,
                                           std::unique_ptr<qtrade::strategy::IStrategy> strategy,
                                           const std::vector<std::string>& instruments) {
  if (strategy_id.empty() || !strategy) {
    return ErrorCode::kInternalError;
  }
  std::lock_guard lock(mutex_);
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
  strategies_.emplace(strategy_id, StrategyEntry{std::move(strategy), true, false, false, instruments});
  return ErrorCode::kSuccess;
}

ErrorCode StrategyEngine::RegisterFactory(const std::string& plugin, StrategyFactory factory) {
  if (plugin.empty() || !factory) {
    return ErrorCode::kInternalError;
  }
  std::lock_guard lock(mutex_);
  return factories_.emplace(plugin, std::move(factory)).second ? ErrorCode::kSuccess : ErrorCode::kSystemError;
}

ErrorCode StrategyEngine::ApplyConfiguration(const std::vector<StrategyRuntimeConfig>& configs) {
  std::lock_guard lock(mutex_);

  // 1. 预检：策略 ID 唯一、工厂可得、启用策略品种不冲突
  std::unordered_set<std::string> configured_ids;
  std::unordered_set<std::string> routed_instruments;
  for (const auto& config : configs) {
    if (config.strategy_id.empty() || !configured_ids.insert(config.strategy_id).second) {
      return ErrorCode::kSystemError;
    }
    if (!strategies_.contains(config.strategy_id) && !factories_.contains(config.plugin)) {
      return ErrorCode::kNotFound;
    }
    if (config.enabled) {
      for (const auto& instrument : config.instruments) {
        if (instrument.empty() || !routed_instruments.insert(instrument).second) {
          return ErrorCode::kSystemError;
        }
      }
    }
  }

  // 2. 创建缺失实例、应用参数并按配置启停
  for (const auto& config : configs) {
    auto entry_it = strategies_.find(config.strategy_id);
    if (entry_it == strategies_.end()) {
      auto strategy = factories_.at(config.plugin)();
      if (!strategy) {
        return ErrorCode::kInternalError;
      }
      qtrade::strategy::StrategyConfig init_config;
      init_config.name = config.strategy_id;
      init_config.parameter_blob = nlohmann::json(config.params).dump();
      if (strategy->Init(init_config) != ErrorCode::kSuccess) {
        return ErrorCode::kInternalError;
      }
      entry_it =
        strategies_.emplace(config.strategy_id, StrategyEntry{std::move(strategy), false, false, true, {}}).first;
    }

    StrategyEntry& entry = entry_it->second;
    for (const auto& [key, value] : config.params) {
      if (entry.strategy->SetParameter(key, value) != ErrorCode::kSuccess) {
        return ErrorCode::kInternalError;
      }
    }

    entry.managed = true;
    if (running_) {
      if (config.enabled && (!entry.enabled || !entry.started)) {
        if (entry.started) {
          entry.strategy->Resume();
        } else if (entry.strategy->Start() == ErrorCode::kSuccess) {
          entry.started = true;
        } else {
          return ErrorCode::kInternalError;
        }
      } else if (!config.enabled && entry.enabled && entry.started) {
        entry.strategy->Pause();
      }
    }
    entry.enabled = config.enabled;
    entry.instruments = config.instruments;
  }

  // 3. 暂停本次配置未覆盖的托管策略，并重建品种路由表
  for (auto& [strategy_id, entry] : strategies_) {
    if (entry.managed && !configured_ids.contains(strategy_id) && entry.enabled) {
      if (running_ && entry.started) {
        entry.strategy->Pause();
      }
      entry.enabled = false;
      entry.instruments.clear();
    }
  }

  instrument_routes_.clear();
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (!entry.enabled) {
      continue;
    }
    for (const auto& instrument : entry.instruments) {
      instrument_routes_[instrument] = entry.strategy.get();
    }
  }
  return ErrorCode::kSuccess;
}

void StrategyEngine::SetOrderSender(OrderSender sender) {
  std::lock_guard<std::mutex> lock(mutex_);
  order_sender_ = std::move(sender);
}

void StrategyEngine::OnTickEvent(const qtrade_sdk::quote::MarketTick& tick) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return;
  }
  // 有品种路由则单播，否则广播给全部启用策略
  if (const auto route = instrument_routes_.find(tick.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] routed strategy OnTick exception: {}", e.what());
    }
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (!entry.enabled) {
      continue;
    }
    try {
      entry.strategy->OnTick(tick);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnTick exception: {}", e.what());
    }
  }
}

void StrategyEngine::OnBarEvent(const qtrade_sdk::quote::Bar& bar) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return;
  }
  // 有品种路由则单播，否则广播给全部启用策略
  if (const auto route = instrument_routes_.find(bar.instrument); route != instrument_routes_.end()) {
    try {
      route->second->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] routed strategy OnBar exception: {}", e.what());
    }
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (!entry.enabled) {
      continue;
    }
    try {
      entry.strategy->OnBar(bar);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnBar exception: {}", e.what());
    }
  }
}

void StrategyEngine::OnOrderEvent(const qtrade_sdk::trader::Order& order) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (!entry.enabled) {
      continue;
    }
    try {
      entry.strategy->OnOrder(order);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnOrder exception: {}", e.what());
    }
  }
}

void StrategyEngine::OnTradeEvent(const qtrade_sdk::trader::Trade& trade) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return;
  }
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (!entry.enabled) {
      continue;
    }
    try {
      entry.strategy->OnTrade(trade);
    } catch (const std::exception& e) {
      spdlog::error("[StrategyEngine] strategy OnTrade exception: {}", e.what());
    }
  }
}

}  // namespace qtrade::engine::strategy
