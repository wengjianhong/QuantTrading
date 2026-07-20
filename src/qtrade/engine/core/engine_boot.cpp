/// @file      engine_boot.cpp
/// @brief     交易引擎进程启动阶段实现（业务相关）
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/engine_boot.hpp"

#include "qtrade/common/app/process_boot.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "strategy/example_strategy.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::boot {
namespace {

constexpr const char* kServiceName = "qtrade_engine";

}  // namespace

bool LoadBootstrapConfig(TradingEngine& engine, const std::string& config_path) {
  spdlog::info("[engine_boot] LoadBootstrapConfig path={}", config_path);
  const ErrorCode error_code = engine.ReloadFromJson(config_path);
  if (error_code != ErrorCode::kSuccess) {
    spdlog::warn("[engine_boot] LoadBootstrapConfig failed, code={}, using defaults", static_cast<int>(error_code));
  }
  return true;
}

bool RegisterDemoStrategies(TradingEngine& engine) {
  spdlog::info("[engine_boot] RegisterDemoStrategies");

  auto order_sender = [&engine](const qtrade_sdk::trader::OrderRequest& request) {
    return engine.SubmitOrder(request);
  };
  auto& strategy_engine = engine.GetStrategyEngine();
  auto example_factory = [&order_sender] {
    auto strategy = qtrade::demo::CreateExampleStrategy();
    static_cast<qtrade::demo::ExampleStrategy*>(strategy.get())->SetOrderSender(order_sender);
    return strategy;
  };

  if (strategy_engine.RegisterFactory("example", example_factory) != ErrorCode::kSuccess ||
      strategy_engine.RegisterFactory("example_strategy", example_factory) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] RegisterDemoStrategies: factory register failed");
    return false;
  }

  auto strategy = example_factory();
  qtrade::strategy::StrategyConfig strategy_config;
  strategy_config.name = "ExampleStrategy";
  if (strategy->Init(strategy_config) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] RegisterDemoStrategies: strategy Init failed");
    return false;
  }
  if (strategy_engine.RegisterStrategy("demo-example", std::move(strategy)) != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] RegisterDemoStrategies: strategy register failed");
    return false;
  }
  strategy_engine.SetOrderSender(order_sender);
  return true;
}

bool InitEngine(TradingEngine& engine) {
  spdlog::info("[engine_boot] InitEngine");
  const ErrorCode error_code = engine.Init();
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
  auto& quote_normalizer = engine.GetQuoteNormalizer();
  auto* quote_api = quote_normalizer.GetQuoteApi();
  if (quote_api != nullptr && quote_api->IsConnected()) {
    quote_normalizer.Subscribe({"IF2401", "IC2401"});
  }
  return true;
}

void RunUntilShutdown(TradingEngine& engine) {
  spdlog::info("[engine_boot] RunUntilShutdown");
  (void)qtrade::common::process_boot::WaitForInterruptAndNotifyStopping(kServiceName);

  (void)engine.Stop();
  qtrade::common::process_boot::LogProcessStopped(kServiceName);
}

}  // namespace qtrade::engine::boot
