/// @file      strategy_manager.cpp
/// @brief     策略管理器实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "strategy_manager.hpp"

#include "qtrade/common/converter/strategy_config_converter.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::strategy {

StrategyManager::StrategyManager(event_bus::EventLanes& event_lanes) : event_lanes_(event_lanes) {}

StrategyManager::~StrategyManager() {
  Stop();
}

void StrategyManager::RebuildStrategyListLocked() {
  strategy_list_.clear();
  strategy_list_.reserve(strategies_.size());
  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    strategy_list_.push_back(entry.strategy.get());
  }
}

ErrorCode StrategyManager::Init(
  const std::string& plugin_dir,
  const google::protobuf::RepeatedPtrField<qtrade::config::v1::StrategyConfig>& strategies,
  OrderSender order_sender) {
  {
    std::lock_guard lock(mutex_);
    if (running_.load()) {
      return ErrorCode::kAlreadyStarted;
    }
  }

  if (plugin_loader_.LoadStrategyPlugin(plugin_dir) != ErrorCode::kSuccess) {
    spdlog::error("[StrategyManager] LoadStrategyPlugin failed dir={}", plugin_dir);
    return ErrorCode::kDynamicLibraryLoadError;
  }

  if (strategies.empty()) {
    spdlog::warn("[StrategyManager] strategies is empty");
  }

  for (const auto& config : strategies) {
    if (!config.enabled()) {
      spdlog::info("[StrategyManager] skip disabled strategy {}", config.strategy_id());
      continue;
    }

    auto strategy = plugin_loader_.Create(config.strategy_name());
    if (!strategy) {
      spdlog::error(
        "[StrategyManager] unknown strategy_name={} strategy_id={}", config.strategy_name(), config.strategy_id());
      return ErrorCode::kDynamicLibrarySymbolNotFound;
    }
    strategy->SetOrderSender(order_sender);

    const auto init_config = qtrade::common::converter::ParseStrategyConfigProto(config);
    if (strategy->Init(init_config) != ErrorCode::kSuccess) {
      spdlog::error("[StrategyManager] strategy Init failed strategy_id={}", config.strategy_id());
      return ErrorCode::kInternalError;
    }

    if (RegisterStrategy(config.strategy_id(), std::move(strategy), init_config.instruments) != ErrorCode::kSuccess) {
      spdlog::error("[StrategyManager] RegisterStrategy failed strategy_id={}", config.strategy_id());
      return ErrorCode::kSystemError;
    }
  }

  {
    std::lock_guard lock(mutex_);
    RebuildStrategyListLocked();
    dispatcher_ =
      std::make_unique<StrategyEventDispatcher>(event_lanes_, mutex_, running_, instrument_routes_, strategy_list_);
    dispatcher_->Subscribe();
  }

  spdlog::info("[StrategyManager] Init registered {} strategy instance(s) from {} config(s)",
               strategies_.size(),
               strategies.size());
  return ErrorCode::kSuccess;
}

ErrorCode StrategyManager::Start() {
  std::lock_guard lock(mutex_);
  if (running_.load()) {
    return ErrorCode::kAlreadyStarted;
  }
  running_.store(true);

  for (auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    if (entry.strategy->Start() != ErrorCode::kSuccess) {
      spdlog::error("[StrategyManager] strategy Start failed");
    }
  }

  spdlog::info("[StrategyManager] started with {} strategies", strategies_.size());
  return ErrorCode::kSuccess;
}

void StrategyManager::Stop() {
  {
    std::lock_guard lock(mutex_);
    if (running_.load()) {
      for (auto& [strategy_id, entry] : strategies_) {
        (void)strategy_id;
        entry.strategy->Stop();
      }
      running_.store(false);
    }
    strategies_.clear();
    instrument_routes_.clear();
    strategy_list_.clear();
    dispatcher_.reset();
  }
  plugin_loader_.UnloadAll();
  spdlog::info("[StrategyManager] stopped and cleared");
}

ErrorCode StrategyManager::RegisterStrategy(const std::string& strategy_id,
                                            StrategyPtr strategy,
                                            const std::vector<std::string>& instruments) {
  if (strategy_id.empty() || strategy == nullptr) {
    return ErrorCode::kInvalidArgument;
  }
  std::lock_guard lock(mutex_);
  if (running_.load()) {
    return ErrorCode::kAlreadyStarted;
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
  RebuildStrategyListLocked();
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::engine::strategy
