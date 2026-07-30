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

#include <spdlog/spdlog.h>

#include <vector>

namespace qtrade::engine::boot {
namespace {

[[nodiscard]] qtrade::strategy::StrategyConfig ToStrategyConfig(const qtrade::config::v1::StrategyConfig& config) {
  qtrade::strategy::StrategyConfig out;
  out.strategy_id = config.strategy_id();
  out.strategy_name = config.strategy_name();
  out.enabled = config.enabled();
  out.instruments.assign(config.instruments().begin(), config.instruments().end());
  out.order_volume = config.order_volume();
  out.max_position = config.max_position();
  out.order_cooldown_ms = config.order_cooldown_ms();
  if (config.has_window_size()) {
    out.window_size = config.window_size();
  }
  if (config.has_order_threshold()) {
    out.order_threshold = config.order_threshold();
  }
  if (config.has_stop_loss_percent()) {
    out.stop_loss_percent = config.stop_loss_percent();
  }
  if (config.has_take_profit_percent()) {
    out.take_profit_percent = config.take_profit_percent();
  }
  return out;
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

  // 2. 按 runtime_config.strategies 创建并注册启用中的策略
  const auto runtime_config = engine.GetRuntimeConfig();
  if (runtime_config.strategies_size() == 0) {
    spdlog::warn("[engine_boot] LoadStrategies: runtime_config.strategies is empty");
    return true;
  }

  qtrade::strategy::OrderSender order_sender = [&engine](const qtrade::strategy::OrderBatch& batch) {
    ErrorCode last = ErrorCode::kSuccess;
    for (const auto& request : batch.order_requests) {
      last = engine.SubmitOrder(request);
      if (last != ErrorCode::kSuccess) {
        return last;
      }
    }
    return last;
  };

  for (const auto& config : runtime_config.strategies()) {
    if (!config.enabled()) {
      spdlog::info("[engine_boot] LoadStrategies: skip disabled strategy {}", config.strategy_id());
      continue;
    }
    auto strategy = plugin_loader.Create(config.strategy_name());
    if (!strategy) {
      spdlog::error("[engine_boot] LoadStrategies: unknown strategy_name={} strategy_id={}",
                    config.strategy_name(),
                    config.strategy_id());
      return false;
    }
    strategy->SetOrderSender(order_sender);

    const auto init_config = ToStrategyConfig(config);
    if (strategy->Init(init_config) != ErrorCode::kSuccess) {
      spdlog::error("[engine_boot] LoadStrategies: Init failed strategy_id={}", config.strategy_id());
      return false;
    }

    if (strategy_manager.RegisterStrategy(config.strategy_id(), std::move(strategy), init_config.instruments) !=
        ErrorCode::kSuccess) {
      spdlog::error("[engine_boot] LoadStrategies: RegisterStrategy failed strategy_id={}", config.strategy_id());
      return false;
    }
  }

  spdlog::info("[engine_boot] LoadStrategies: registered enabled strategies from {} config(s)",
               runtime_config.strategies_size());
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
