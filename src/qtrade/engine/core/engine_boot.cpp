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

namespace qtrade::engine::boot {

bool RegisterStrategies(TradingEngine& engine) {
  spdlog::info("[engine_boot] RegisterStrategies");

  // 1. 构造发单桥接：策略 → engine.SubmitOrder
  auto order_sender = [&engine](const qtrade_sdk::trader::OrderRequest& request) {
    return engine.SubmitOrder(request);
  };
  auto& strategy_engine = engine.GetStrategyEngine();
  auto example_factory = [&order_sender] {
    auto strategy = qtrade::demo::CreateExampleStrategy();
    static_cast<qtrade::demo::ExampleStrategy*>(strategy.get())->SetOrderSender(order_sender);
    return strategy;
  };

  // 2. 注册示例策略工厂（支持 config 中 plugin=example / example_strategy）
  if (strategy_engine.RegisterFactory("example", example_factory) != ErrorCode::kSuccess ||
      strategy_engine.RegisterFactory("example_strategy", example_factory) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] RegisterStrategies: factory register failed");
    return false;
  }

  // 3. 预注册一个 demo 实例并注入全局发单器
  auto strategy = example_factory();
  qtrade::strategy::StrategyConfig strategy_config;
  strategy_config.name = "ExampleStrategy";
  if (strategy->Init(strategy_config) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] RegisterStrategies: strategy Init failed");
    return false;
  }
  if (strategy_engine.RegisterStrategy("demo-example", std::move(strategy)) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] RegisterStrategies: strategy register failed");
    return false;
  }
  strategy_engine.SetOrderSender(order_sender);
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

  // 演示订阅：生产环境应由 config-service 下发的策略 instruments 驱动
  if (auto* quote_api = engine.GetQuoteApi(); quote_api != nullptr && quote_api->IsConnected()) {
    engine.SubscribeQuote({"IF2401", "IC2401"});
  }
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
