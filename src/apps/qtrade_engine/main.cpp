/// @file      main.cpp
/// @brief     交易引擎独立进程入口（qtrade_engine）
/// @details   解析配置、初始化日志与交易引擎，挂载示例策略并运行主循环
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "adapter/mock/quote/mock_quote_api.hpp"
#include "common/app/app_runner.hpp"
#include "common/logging/logger.hpp"
#include "engine/trading_engine.hpp"
#include "strategy/example_strategy.hpp"

#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char** argv) {
  std::string config_path;
  if (!qtrade::common::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_engine] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  if (!qtrade::common::init_spdlog_logger("logs", "trading-engine.log")) {
    return EXIT_FAILURE;
  }

  spdlog::info("==================================================");
  spdlog::info("qtrade_engine starting (pid={})", getpid());
  spdlog::info("config: {}", config_path);
  spdlog::info("==================================================");

  qtrade::engine::TradingEngine engine;
  if (const auto rc = engine.ReloadFromJson(config_path); rc != qtrade::ErrorCode::kSuccess) {
    spdlog::warn("[qtrade_engine] engine config load failed, using defaults");
  }

  std::atomic<bool> stop_flag{false};
  qtrade::common::InstallShutdownHandler(stop_flag);

  if (const auto rc = engine.Init(); rc != qtrade::ErrorCode::kSuccess) {
    spdlog::error("[qtrade_engine] init failed, code={}", static_cast<int>(rc));
    return EXIT_FAILURE;
  }

  auto& quote_normalizer = engine.GetQuoteNormalizer();
  auto& strategy_engine = engine.GetStrategyEngine();

  auto quote_api = qtrade::adapter::mock::quote::CreateMockQuoteApi();
  quote_normalizer.SetQuoteApi(std::move(quote_api));

  auto strategy = qtrade::demo::CreateExampleStrategy();
  qtrade::strategy::StrategyConfig strategy_cfg;
  strategy_cfg.name = "ExampleStrategy";
  strategy->Init(strategy_cfg);

  auto* example_strategy = static_cast<qtrade::demo::ExampleStrategy*>(strategy.get());
  auto order_sender = [](const qtrade_sdk::trader::OrderRequest& request) {
    spdlog::info("[OrderSender] {} {} {} @ {}",
                 request.instrument,
                 request.side == qtrade_sdk::trader::SideType::kBuy ? "BUY" : "SELL",
                 request.volume,
                 request.price);
    return qtrade::ErrorCode::kSuccess;
  };
  example_strategy->SetOrderSender(order_sender);
  strategy_engine.RegisterStrategy(std::move(strategy));
  strategy_engine.SetOrderSender(order_sender);

  if (const auto rc = engine.Start(); rc != qtrade::ErrorCode::kSuccess) {
    spdlog::error("[qtrade_engine] start failed, code={}", static_cast<int>(rc));
    return EXIT_FAILURE;
  }

  if (auto* source_ptr = quote_normalizer.GetQuoteApi()) {
    qtrade_sdk::quote::ConnectRequest source_cfg;
    source_cfg.name = "MockDataSource";
    source_cfg.connection_string = "mock://localhost";
    source_ptr->Connect(source_cfg);
    quote_normalizer.Subscribe({"IF2401", "IC2401"});
  }

  qtrade::common::UnblockShutdownSignals();

  spdlog::info("[qtrade_engine] running until SIGINT/SIGTERM...");

  qtrade::common::RunUntilStop(stop_flag);

  engine.Stop();
  spdlog::info("[qtrade_engine] stopped cleanly");
  return EXIT_SUCCESS;
}
