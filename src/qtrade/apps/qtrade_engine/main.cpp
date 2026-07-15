/// @file      main.cpp
/// @brief     交易引擎独立进程入口（qtrade_engine）
/// @details   解析配置、初始化日志与交易引擎，挂载示例策略并运行主循环
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
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

int main(int argc, char** argv) {
  /// 解析 --config
  std::string config_path;
  if (!qtrade::common::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_engine] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  /// 初始化日志
  if (!qtrade::common::init_spdlog_logger("logs", "trading-engine.log")) {
    std::cerr << "failed to initialize logger, service_name=qtrade_engine" << std::endl;
    return EXIT_FAILURE;
  }
  spdlog::info("======================= Starting =========================");
  spdlog::info("service_name=qtrade_engine");
  spdlog::info("config_path={}", config_path);
  spdlog::info("service_pid={}", getpid());

  /// 加载引擎配置（失败则继续用默认值）
  qtrade::engine::TradingEngine engine;
  qtrade::ErrorCode error_code = engine.ReloadFromJson(config_path);
  if (error_code != qtrade::ErrorCode::kSuccess) {
    spdlog::warn("engine config load failed, code={}, using defaults", static_cast<int>(error_code));
  }

  /// 安装退出信号处理器（须在业务线程起来前）
  std::atomic<bool> stop_flag{false};
  qtrade::common::InstallShutdownHandler(stop_flag);

  /// 初始化引擎内部模块
  error_code = engine.Init();
  if (error_code != qtrade::ErrorCode::kSuccess) {
    spdlog::error("init failed, code={}", static_cast<int>(error_code));
    return EXIT_FAILURE;
  }

  /// 挂载 demo：Mock 行情源 + 示例策略 + 打日志发单
  auto& quote_normalizer = engine.GetQuoteNormalizer();
  auto& trader_normalizer = engine.GetTraderNormalizer();
  auto& strategy_engine = engine.GetStrategyEngine();

  auto quote_api = qtrade::adapter::mock::quote::CreateMockQuoteApi();
  quote_normalizer.SetQuoteApi(std::move(quote_api));
  trader_normalizer.SetTraderApi(qtrade::adapter::mock::trader::CreateMockTraderApi());
  if (auto* trader_api = trader_normalizer.GetTraderApi()) {
    qtrade_sdk::trader::ConnectRequest trader_config;
    trader_config.broker_id = "mock";
    trader_config.connection_string = "mock://localhost";
    trader_api->Connect(trader_config);
  }

  auto strategy = qtrade::demo::CreateExampleStrategy();
  qtrade::strategy::StrategyConfig strategy_cfg;
  strategy_cfg.name = "ExampleStrategy";
  strategy->Init(strategy_cfg);

  auto* example_strategy = static_cast<qtrade::demo::ExampleStrategy*>(strategy.get());
  auto order_sender = [&engine](const qtrade_sdk::trader::OrderRequest& request) {
    return engine.SubmitOrder(request);
  };
  example_strategy->SetOrderSender(order_sender);
  strategy_engine.RegisterStrategy(std::move(strategy));
  strategy_engine.SetOrderSender(order_sender);

  /// 启动引擎事件循环
  if (const auto rc = engine.Start(); rc != qtrade::ErrorCode::kSuccess) {
    spdlog::error("[qtrade_engine] start failed, code={}", static_cast<int>(rc));
    return EXIT_FAILURE;
  }

  /// 连接 Mock 行情并订阅示例合约
  if (auto* source_ptr = quote_normalizer.GetQuoteApi()) {
    qtrade_sdk::quote::ConnectRequest source_cfg;
    source_cfg.name = "MockDataSource";
    source_cfg.connection_string = "mock://localhost";
    source_ptr->Connect(source_cfg);
    quote_normalizer.Subscribe({"IF2401", "IC2401"});
  }

  /// 主线程放开信号并阻塞直至 SIGINT/SIGTERM
  qtrade::common::UnblockShutdownSignals();

  spdlog::info("[qtrade_engine] running until SIGINT/SIGTERM...");

  qtrade::common::RunUntilStop(stop_flag);

  /// 优雅停止引擎
  engine.Stop();
  spdlog::info("service_pid={}", getpid());
  spdlog::info("service_name=qtrade_engine");
  spdlog::info("======================= Stopped =========================");
  return EXIT_SUCCESS;
}
