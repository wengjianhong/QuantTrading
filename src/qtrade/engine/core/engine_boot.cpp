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
#include "strategy/example_strategy.hpp"

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

  // 1. 发单桥接：策略 → Engine.SubmitOrder（READY 门禁后转 OrderPipeline）
  auto order_sender = [&engine](const qtrade_sdk::trader::OrderRequest& request) {
    return engine.SubmitOrder(request);
  };
  strategy_manager.SetOrderSender(order_sender);

  // 2. 注册内置策略工厂（plugin 名与 config-service 下发一致）
  auto example_factory = [order_sender] {
    auto strategy = qtrade::demo::CreateExampleStrategy();
    static_cast<qtrade::demo::ExampleStrategy*>(strategy.get())->SetOrderSender(order_sender);
    return strategy;
  };
  if (strategy_manager.RegisterFactory("example", example_factory) != ErrorCode::kSuccess ||
      strategy_manager.RegisterFactory("example_strategy", example_factory) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] LoadStrategies: factory register failed");
    return false;
  }

  // 3. 按 runtime_config 装配策略实例
  const auto runtime_config = engine.GetRuntimeConfig();
  const auto strategy_configs = BuildStrategyRuntimeConfigs(runtime_config);
  if (strategy_configs.empty()) {
    spdlog::warn("[engine_boot] LoadStrategies: runtime_config.strategies is empty");
    return true;
  }
  if (strategy_manager.ApplyConfiguration(strategy_configs) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] LoadStrategies: ApplyConfiguration failed");
    return false;
  }
  spdlog::info("[engine_boot] LoadStrategies: applied {} strategy config(s)", strategy_configs.size());
  return true;
}

bool InitEngine(TradingEngine& engine, const qtrade::common::process_boot::ProgramOptions& options) {
  // 1. 加载配置文件
  const auto config_node = qtrade::common::LoadJsonFile(options.config_path);
  if (!config_node) {
    spdlog::error("[engine_boot] InitEngine: failed to load config file");
    return false;
  }

  // 2. 解析配置文件
  const auto config = qtrade::common::config::ParseQtradeEngineBootstrapConfig(config_node.value());
  if (!config) {
    spdlog::error("[engine_boot] InitEngine: failed to parse config");
    return false;
  }

  // 3. 初始化引擎
  const ErrorCode error_code = engine.Init(config.value());
  if (error_code != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] InitEngine failed, code={}", static_cast<int>(error_code));
    return false;
  }

  return true;
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
