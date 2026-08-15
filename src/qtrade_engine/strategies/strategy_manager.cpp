/// @file      strategy_manager.cpp
/// @brief     策略管理器实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "strategy_manager.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>

namespace qtrade::engine::strategies {

StrategyManager::StrategyManager(events::EventLanes& event_lanes) : event_lanes_(event_lanes) {}

StrategyManager::~StrategyManager() {
  Stop();
}

std::vector<IStrategy*> StrategyManager::BuildStrategyListLocked() const {
  std::vector<IStrategy*> list;
  list.reserve(strategies_.size());
  for (const auto& [strategy_id, entry] : strategies_) {
    (void)strategy_id;
    list.push_back(entry.strategy.get());
  }
  return list;
}

void StrategyManager::PushRoutingToDispatcherLocked() {
  if (!dispatcher_) {
    return;
  }
  dispatcher_->SetRouting(instrument_routes_, BuildStrategyListLocked());
}

ErrorCode StrategyManager::AddStrategyFromPlugin(const StrategyConfig& config,
                                                 const std::string& plugin_so_path,
                                                 OrderSender order_sender) {
  {
    std::lock_guard lock(mutex_);
    if (running_.load()) {
      return ErrorCode::kAlreadyStarted;
    }
  }

  if (config.strategy_id.empty() || config.strategy_name.empty()) {
    return ErrorCode::kInvalidArgument;
  }
  if (!config.enabled) {
    spdlog::info("[StrategyManager] skip disabled strategy {}", config.strategy_id);
    return ErrorCode::kSuccess;
  }
  if (plugin_so_path.empty()) {
    spdlog::error("[StrategyManager] empty plugin_so_path strategy_id={}", config.strategy_id);
    return ErrorCode::kInvalidArgument;
  }
  {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(plugin_so_path, ec)) {
      spdlog::error("[StrategyManager] plugin so not found path={} strategy_id={} ({})",
                    plugin_so_path,
                    config.strategy_id,
                    ec.message());
      return ErrorCode::kNotSuchFileOrDirectory;
    }
  }

  // 同插件多实例：已加载则复用句柄，不再次 dlopen
  if (!plugin_loader_.HasPlugin(config.strategy_name)) {
    if (const auto rc = plugin_loader_.LoadFile(plugin_so_path); rc != ErrorCode::kSuccess) {
      spdlog::error("[StrategyManager] LoadFile failed path={} strategy_id={}",
                    plugin_so_path,
                    config.strategy_id);
      return rc;
    }
    if (!plugin_loader_.HasPlugin(config.strategy_name)) {
      spdlog::error(
        "[StrategyManager] plugin ABI name mismatch: config.strategy_name={} path={}",
        config.strategy_name,
        plugin_so_path);
      return ErrorCode::kDynamicLibrarySymbolNotFound;
    }
  }

  auto strategy = plugin_loader_.Create(config.strategy_name);
  if (!strategy) {
    spdlog::error(
      "[StrategyManager] Create failed strategy_name={} strategy_id={}", config.strategy_name, config.strategy_id);
    return ErrorCode::kDynamicLibrarySymbolNotFound;
  }
  strategy->SetOrderSender(std::move(order_sender));

  if (strategy->Init(config) != ErrorCode::kSuccess) {
    spdlog::error("[StrategyManager] strategy Init failed strategy_id={}", config.strategy_id);
    return ErrorCode::kInternalError;
  }

  if (RegisterStrategy(config.strategy_id, std::move(strategy), config.instruments) != ErrorCode::kSuccess) {
    spdlog::error("[StrategyManager] RegisterStrategy failed strategy_id={}", config.strategy_id);
    return ErrorCode::kSystemError;
  }

  spdlog::info("[StrategyManager] registered strategy_id={} name={} path={}",
               config.strategy_id,
               config.strategy_name,
               plugin_so_path);
  return ErrorCode::kSuccess;
}

ErrorCode StrategyManager::Start() {
  std::lock_guard lock(mutex_);
  if (running_.load()) {
    return ErrorCode::kAlreadyStarted;
  }

  // 1. 无 Init 的单测路径：按当前注册表补建分发器
  if (!dispatcher_) {
    dispatcher_ = std::make_unique<StrategyEventDispatcher>(event_lanes_);
    PushRoutingToDispatcherLocked();
    dispatcher_->Subscribe();
  }

  // 2. 激活投递并逐个 Start 策略
  dispatcher_->SetActive(true);
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
  // 1. 先停投递并销毁分发器，避免回调触达已析构策略
  {
    std::lock_guard lock(mutex_);
    if (dispatcher_) {
      dispatcher_->SetActive(false);
      dispatcher_.reset();
    }

    if (running_.load()) {
      for (auto& [strategy_id, entry] : strategies_) {
        (void)strategy_id;
        entry.strategy->Stop();
      }
      running_.store(false);
    }
    strategies_.clear();
    instrument_routes_.clear();
  }

  // 2. 卸载全部插件句柄
  plugin_loader_.UnloadAll();
  spdlog::info("[StrategyManager] stopped and cleared");
}

ErrorCode StrategyManager::RegisterStrategy(const std::string& strategy_id,
                                            StrategyPtr strategy,
                                            const std::vector<std::string>& instruments) {
  // 1. 校验参数与运行态
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

  // 2. 校验合约路由无冲突后写入注册表
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

  // 3. 若分发器已存在（Init 后、Start 前补注册），刷新快照
  PushRoutingToDispatcherLocked();
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::engine::strategies
