/// @file      engine_boot.cpp
/// @brief     交易引擎进程启动阶段实现（业务相关）
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/engine_boot.hpp"

#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"
#include "qtrade/common/json/json_util.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "qtrade/engine/trading_engine_define.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <vector>

namespace qtrade::engine::boot {
namespace {

std::vector<strategy::StrategyRuntimeConfig> BuildStrategyRuntimeConfigs(
  const qtrade::config::v1::EngineConfig& runtime_config) {
  std::vector<strategy::StrategyRuntimeConfig> configs;
  configs.reserve(static_cast<std::size_t>(runtime_config.strategies_size()));
  for (const auto& strategy : runtime_config.strategies()) {
    strategy::StrategyRuntimeConfig item;
    item.strategy_id = strategy.strategy_id();
    item.plugin = strategy.plugin();
    item.enabled = strategy.enabled();
    item.instruments.assign(strategy.instruments().begin(), strategy.instruments().end());
    item.params.insert(strategy.params().begin(), strategy.params().end());
    configs.push_back(std::move(item));
  }
  return configs;
}

}  // namespace

bool LoadStrategies(TradingEngine& engine) {
  spdlog::info("[engine_boot] LoadStrategies");

  auto& strategy_manager = engine.GetStrategyManager();
  auto& plugin_loader = engine.GetStrategyPluginLoader();

  // 1. 扫描策略插件目录
  const auto& plugin_dir = engine.GetConfig().config.strategy.plugin_dir;
  if (plugin_loader.LoadDirectory(plugin_dir) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] LoadStrategies: LoadDirectory failed dir={}", plugin_dir);
    return false;
  }

  // 2. 按 runtime_config 创建并注册启用中的策略
  const auto runtime_config = engine.GetRuntimeConfig();
  const auto strategy_configs = BuildStrategyRuntimeConfigs(runtime_config);
  if (strategy_configs.empty()) {
    spdlog::warn("[engine_boot] LoadStrategies: runtime_config.strategies is empty");
    return true;
  }

  qtrade::strategy::OrderSender order_sender = [&engine](const qtrade_sdk::trader::OrderRequest& request) {
    return engine.SubmitOrder(request);
  };

  for (const auto& config : strategy_configs) {
    if (!config.enabled) {
      spdlog::info("[engine_boot] LoadStrategies: skip disabled strategy {}", config.strategy_id);
      continue;
    }
    auto strategy = plugin_loader.Create(config.plugin);
    if (!strategy) {
      spdlog::error(
        "[engine_boot] LoadStrategies: unknown plugin={} strategy_id={}", config.plugin, config.strategy_id);
      return false;
    }
    strategy->SetOrderSender(order_sender);

    qtrade::strategy::StrategyConfig init_config;
    init_config.name = config.strategy_id;
    init_config.parameter_blob = nlohmann::json(config.params).dump();
    if (strategy->Init(init_config) != ErrorCode::kSuccess) {
      spdlog::error("[engine_boot] LoadStrategies: Init failed strategy_id={}", config.strategy_id);
      return false;
    }
    for (const auto& [key, value] : config.params) {
      if (strategy->SetParameter(key, value) != ErrorCode::kSuccess) {
        spdlog::error(
          "[engine_boot] LoadStrategies: SetParameter failed strategy_id={} key={}", config.strategy_id, key);
        return false;
      }
    }

    if (strategy_manager.RegisterStrategy(config.strategy_id, std::move(strategy), config.instruments) !=
        ErrorCode::kSuccess) {
      spdlog::error("[engine_boot] LoadStrategies: RegisterStrategy failed strategy_id={}", config.strategy_id);
      return false;
    }
  }

  spdlog::info("[engine_boot] LoadStrategies: registered enabled strategies from {} config(s)",
               strategy_configs.size());
  return true;
}

bool InitEngine(TradingEngine& engine, const qtrade::common::config::QtradeEngineBootstrapConfig& config) {
  spdlog::info("[engine_boot] InitEngine");
  const ErrorCode error_code = engine.Init(config);
  if (error_code != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] InitEngine failed, code={}", static_cast<int>(error_code));
    return false;
  }
  return true;
}

std::optional<qtrade::common::config::QtradeEngineBootstrapConfig> LoadBootstrapConfig(
  const qtrade::common::process_boot::ProgramOptions& options) {
  const auto config_node = qtrade::common::LoadJsonFile(options.config_path);
  if (!config_node) {
    spdlog::error("[engine_boot] LoadBootstrapConfig: failed to load config file");
    return std::nullopt;
  }
  const auto config = qtrade::common::config::ParseQtradeEngineBootstrapConfig(config_node.value());
  if (!config) {
    spdlog::error("[engine_boot] LoadBootstrapConfig: failed to parse config");
    return std::nullopt;
  }
  return config;
}

bool StartEngine(TradingEngine& engine) {
  spdlog::info("[engine_boot] StartEngine");
  const ErrorCode error_code = engine.Start();
  if (error_code != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] StartEngine failed, code={}", static_cast<int>(error_code));
    return false;
  }
  // 行情订阅由 StartMarketData 按 runtime_config 已启用策略的 instruments 驱动
  return true;
}

void RunUntilShutdown(TradingEngine& engine) {
  // 阻塞等待 SIGINT/SIGTERM，收到信号后优雅 Stop
  spdlog::info("[{}] running until SIGINT/SIGTERM...", qtrade::engine::kServiceName);

  const int signal = qtrade::common::system::WaitInterruptSignals();
  spdlog::info("[{}] received signal {}, stopping...", qtrade::engine::kServiceName, signal);

  (void)engine.Stop();
}

}  // namespace qtrade::engine::boot
