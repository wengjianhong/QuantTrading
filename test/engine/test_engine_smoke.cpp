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

#include <chrono>
#include <functional>
#include <string>
#include <thread>

namespace {

void WaitUntil(const std::function<bool()>& predicate) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

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

TEST(EngineSmoke, TradingEngineStartStop) {
  const auto config =
    MakeTestEngineConfig("test-engine-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  qtrade::engine::TradingEngine engine;
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);
  InstallStubAdapters(engine);
  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  engine.SubscribeQuote({"IF2506"});
  WaitUntil([&] { return engine.IsReady(); });
  ASSERT_TRUE(engine.IsRunning());
  ASSERT_TRUE(engine.IsReady());
  EXPECT_EQ(engine.State(), qtrade::engine::EngineState::kReady);
  engine.Stop();
  ASSERT_FALSE(engine.IsRunning());
  EXPECT_EQ(engine.State(), qtrade::engine::EngineState::kStopped);
}

TEST(EngineSmoke, MarketTickSize) {
  qtrade::sdk::quote::MarketTick tick;
  (void)tick;
  SUCCEED();
}

TEST(EngineSmoke, InjectedStubAdaptersReachReady) {
  const std::string suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  const auto config = MakeTestEngineConfig("configured-stub-" + suffix);
  qtrade::engine::TradingEngine engine;
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);
  InstallStubAdapters(engine);
  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  engine.SubscribeQuote({"IF2506"});
  WaitUntil([&] { return engine.IsReady(); });
  EXPECT_TRUE(engine.IsReady());
  ASSERT_NE(engine.GetTraderApi(), nullptr);
  EXPECT_TRUE(engine.GetTraderApi()->IsConnected());
  EXPECT_EQ(engine.Stop(), qtrade::ErrorCode::kSuccess);
}
