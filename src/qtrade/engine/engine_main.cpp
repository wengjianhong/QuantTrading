/// @file      engine_main.cpp
/// @brief     交易引擎进程入口实现
/// @details   负责配置解析、日志初始化、演示适配器装配及进程生命周期管理
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/engine_main.hpp"

#include "qtrade/common/app/app_runner.hpp"
#include "qtrade/common/logging/logger.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "qtrade_sdk/mock/quote/mock_quote_api.hpp"
#include "qtrade_sdk/mock/trader/mock_trader_api.hpp"
#include "strategy/example_strategy.hpp"

#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

namespace qtrade::engine {

int RunTradingEngineMain(int argc, char** argv) {
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

  TradingEngine engine;
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
  auto& trader_normalizer = engine.GetTraderNormalizer();
  auto& strategy_engine = engine.GetStrategyEngine();

  quote_normalizer.SetQuoteApi(qtrade::adapter::mock::quote::CreateMockQuoteApi());
  trader_normalizer.SetTraderApi(qtrade::adapter::mock::trader::CreateMockTraderApi());
  if (auto* trader_api = trader_normalizer.GetTraderApi()) {
    qtrade_sdk::trader::ConnectRequest trader_config;
    trader_config.broker_id = "mock";
    trader_config.connection_string = "mock://localhost";
    trader_api->Connect(trader_config);
  }

  auto strategy = qtrade::demo::CreateExampleStrategy();
  qtrade::strategy::StrategyConfig strategy_config;
  strategy_config.name = "ExampleStrategy";
  strategy->Init(strategy_config);

  auto* example_strategy = static_cast<qtrade::demo::ExampleStrategy*>(strategy.get());
  auto order_sender = [&engine](const qtrade_sdk::trader::OrderRequest& request) {
    return engine.SubmitOrder(request);
  };
  example_strategy->SetOrderSender(order_sender);
  strategy_engine.RegisterStrategy(std::move(strategy));
  strategy_engine.SetOrderSender(order_sender);

  if (const auto result = engine.Start(); result != ErrorCode::kSuccess) {
    spdlog::error("[qtrade_engine] start failed, code={}", static_cast<int>(result));
    return EXIT_FAILURE;
  }

  if (auto* quote_api = quote_normalizer.GetQuoteApi()) {
    qtrade_sdk::quote::ConnectRequest quote_config;
    quote_config.name = "MockDataSource";
    quote_config.connection_string = "mock://localhost";
    quote_api->Connect(quote_config);
    quote_normalizer.Subscribe({"IF2401", "IC2401"});
  }

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
