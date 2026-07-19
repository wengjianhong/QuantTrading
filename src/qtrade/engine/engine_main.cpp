/// @file      engine_main.cpp
/// @brief     交易引擎进程入口实现
/// @details   负责配置解析、日志初始化、演示策略装配及 TradingEngine 进程生命周期管理
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/engine_main.hpp"

#include "qtrade/common/app/app_runner.hpp"
#include "qtrade/common/logging/logger.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "strategy/example_strategy.hpp"

#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

namespace qtrade::engine {

int RunTradingEngineMain(int argc, char** argv) {
  // 1. 解析 --config 并初始化日志
  std::string config_path;
  if (!qtrade::common::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_engine] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  if (!qtrade::common::init_spdlog_logger("logs", "trading-engine.log")) {
    std::cerr << "failed to initialize logger, service_name=qtrade_engine" << std::endl;
    return EXIT_FAILURE;
  }
  spdlog::info("======================= Starting =========================");
  spdlog::info("service_name=qtrade_engine");
  spdlog::info("config_path={}", config_path);
  spdlog::info("service_pid={}", getpid());

  // 2. 装配演示策略工厂与发单回调
  TradingEngine engine;
  auto order_sender = [&engine](const qtrade_sdk::trader::OrderRequest& request) {
    return engine.SubmitOrder(request);
  };
  auto& strategy_engine = engine.GetStrategyEngine();
  auto example_factory = [&order_sender] {
    auto strategy = qtrade::demo::CreateExampleStrategy();
    static_cast<qtrade::demo::ExampleStrategy*>(strategy.get())->SetOrderSender(order_sender);
    return strategy;
  };
  (void)strategy_engine.RegisterFactory("example", example_factory);
  (void)strategy_engine.RegisterFactory("example_strategy", example_factory);

  auto strategy = example_factory();
  qtrade::strategy::StrategyConfig strategy_config;
  strategy_config.name = "ExampleStrategy";
  (void)strategy->Init(strategy_config);
  (void)strategy_engine.RegisterStrategy("demo-example", std::move(strategy));
  strategy_engine.SetOrderSender(order_sender);

  // 3. 加载引导配置并 Init/Start 引擎
  ErrorCode error_code = engine.ReloadFromJson(config_path);
  if (error_code != ErrorCode::kSuccess) {
    spdlog::warn("engine config load failed, code={}, using defaults", static_cast<int>(error_code));
  }

  std::atomic<bool> stop_flag{false};
  qtrade::common::InstallShutdownHandler(stop_flag);

  error_code = engine.Init();
  if (error_code != ErrorCode::kSuccess) {
    spdlog::error("init failed, code={}", static_cast<int>(error_code));
    return EXIT_FAILURE;
  }

  auto& quote_normalizer = engine.GetQuoteNormalizer();

  if (const auto result = engine.Start(); result != ErrorCode::kSuccess) {
    spdlog::error("[qtrade_engine] start failed, code={}", static_cast<int>(result));
    return EXIT_FAILURE;
  }

  if (auto* quote_api = quote_normalizer.GetQuoteApi()) {
    if (quote_api->IsConnected()) {
      quote_normalizer.Subscribe({"IF2401", "IC2401"});
    }
  }

  // 4. 阻塞至停机信号后优雅 Stop
  qtrade::common::UnblockShutdownSignals();
  spdlog::info("[qtrade_engine] running until SIGINT/SIGTERM...");
  qtrade::common::RunUntilStop(stop_flag);

  engine.Stop();
  spdlog::info("service_pid={}", getpid());
  spdlog::info("service_name=qtrade_engine");
  spdlog::info("======================= Stopped =========================");
  return EXIT_SUCCESS;
}

}  // namespace qtrade::engine
