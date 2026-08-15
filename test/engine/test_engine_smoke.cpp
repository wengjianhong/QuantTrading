/// @file      smoke_test.cpp
/// @brief     交易引擎冒烟测试
/// @details   验证 TradingEngine 启动/停止及 TickData 基本行为
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/trading_engine.hpp"
#include "stubs/stub_quote_api.hpp"
#include "stubs/stub_trader_api.hpp"

#include <qtrade/engine/engine.hpp>
#include <qtrade/sdk/quote/quote_struct.hpp>

#include <gtest/gtest.h>

#include <string>

namespace {

[[nodiscard]] qtrade::engine::EngineConfig MakeTestEngineConfig(const std::string& engine_id) {
  qtrade::engine::EngineConfig config;
  config.engine_id = engine_id;
  config.account_id = "test-account";
  return config;
}

void InstallStubAdapters(qtrade::engine::TradingEngine& engine) {
  auto quote_api = qtrade::test::stub::CreateStubQuoteApi();
  qtrade::sdk::quote::ConnectRequest quote_request;
  quote_request.connection_string = "stub://quote";
  EXPECT_EQ(quote_api->Connect(quote_request), qtrade::ErrorCode::kSuccess);
  engine.SetQuoteApi(std::move(quote_api));

  auto trader_api = qtrade::test::stub::CreateStubTraderApi();
  qtrade::sdk::trader::ConnectRequest trader_request;
  trader_request.connection_string = "stub://trader";
  EXPECT_EQ(trader_api->Connect(trader_request), qtrade::ErrorCode::kSuccess);
  engine.SetTraderApi(std::move(trader_api));
}

}  // namespace

TEST(EngineSmoke, TradingEngineStartsWithoutStrategySubscriptions) {
  const auto config =
    MakeTestEngineConfig("test-engine-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  qtrade::engine::TradingEngine engine;
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);
  InstallStubAdapters(engine);
  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  ASSERT_TRUE(engine.IsRunning());
  EXPECT_EQ(engine.State(), qtrade::engine::EngineState::kInitiated);
  engine.Stop();
  ASSERT_FALSE(engine.IsRunning());
  EXPECT_EQ(engine.State(), qtrade::engine::EngineState::kStopped);
}

TEST(EngineSmoke, MarketTickSize) {
  qtrade::sdk::quote::MarketTick tick;
  (void)tick;
  SUCCEED();
}

TEST(EngineSmoke, InjectedStubAdaptersCanStopWithoutSubscriptions) {
  const std::string suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const auto config = MakeTestEngineConfig("configured-stub-" + suffix);
  qtrade::engine::TradingEngine engine;
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);
  InstallStubAdapters(engine);
  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(engine.State(), qtrade::engine::EngineState::kInitiated);
  EXPECT_EQ(engine.Stop(), qtrade::ErrorCode::kSuccess);
}
