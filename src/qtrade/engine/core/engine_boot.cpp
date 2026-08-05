/// @file      engine_boot.cpp
/// @brief     交易引擎进程启动阶段实现（业务相关）
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/engine_boot.hpp"

#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"
#include "qtrade/common/proto/strategy_config_utils.hpp"
#include "qtrade/common/json/json_util.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "qtrade/engine/trading_engine_define.hpp"

#include <spdlog/spdlog.h>

#include <vector>

namespace qtrade::engine::boot {

bool LoadStrategies(TradingEngine& engine) {
  spdlog::info("[engine_boot] LoadStrategies");

  qtrade::strategy::OrderSender order_sender = [&engine](const qtrade::strategy::OrderBatch& batch) {
    if (!engine.IsReady()) {
      return ErrorCode::kNotInitialized;
    }
    return engine.GetOrderPipeline().SubmitBatch(batch);
  };

  // 1. 将 runtime_config.strategies（proto）转为策略运行时配置
  const auto& plugin_dir = engine.GetBootstrapConfig().config.strategy.plugin_dir;
  const auto runtime_config = engine.GetRuntimeConfig();
  std::vector<qtrade::strategy::StrategyConfig> strategies;
  strategies.reserve(static_cast<std::size_t>(runtime_config.strategies_size()));
  for (const auto& proto : runtime_config.strategies()) {
    strategies.push_back(qtrade::common::proto::ParseStrategyConfigProto(proto));
  }

  // 2. 交给 StrategyManager 装载插件并注册实例
  const ErrorCode code = engine.GetStrategyManager().Init(plugin_dir, strategies, std::move(order_sender));
  if (code != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] LoadStrategies: StrategyManager::Init failed, code={}", static_cast<int>(code));
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
  return true;
}

void RunUntilShutdown(TradingEngine& engine) {
  spdlog::info("[{}] running until SIGINT/SIGTERM...", qtrade::engine::kServiceName);

  const int signal = qtrade::common::system::WaitInterruptSignals();
  spdlog::info("[{}] received signal {}, stopping...", qtrade::engine::kServiceName, signal);

  (void)engine.Stop();
}

}  // namespace qtrade::engine::boot
