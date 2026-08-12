#include "qtrade/engine/trading_engine.hpp"
#include "stubs/stub_quote_api.hpp"
#include "stubs/stub_trader_api.hpp"

#include <qtrade/engine/engine.hpp>

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

void InstallConnectedStubQuote(qtrade::engine::TradingEngine& engine) {
  auto quote_api = qtrade::test::stub::CreateStubQuoteApi();
  qtrade::sdk::quote::ConnectRequest request;
  request.connection_string = "stub://quote";
  EXPECT_EQ(quote_api->Connect(request), qtrade::ErrorCode::kSuccess);
  engine.SetQuoteApi(std::move(quote_api));
}

}  // namespace

TEST(OrderPipeline, StubOrderFlowsThroughOmsAndTraderLane) {
  const std::string suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  qtrade::engine::TradingEngine engine;
  ASSERT_EQ(engine.Init(MakeTestEngineConfig("test-engine-" + suffix)), qtrade::ErrorCode::kSuccess);

  qtrade::strategy::StrategyConfig strategy_config;
  strategy_config.enabled = true;
  strategy_config.strategy_id = "test-strategy";
  strategy_config.instruments = {"IF2506"};
  ASSERT_EQ(engine.AddStrategy(strategy_config, ""), qtrade::ErrorCode::kSuccess);

  InstallConnectedStubQuote(engine);
  engine.SetTraderApi(qtrade::test::stub::CreateStubTraderApi());
  qtrade::sdk::trader::ConnectRequest connect_request;
  connect_request.connection_string = "stub://test";
  ASSERT_EQ(engine.GetTraderApi()->Connect(connect_request), qtrade::ErrorCode::kSuccess);

  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  engine.SubscribeQuote({"IF2506"});
  WaitUntil([&] { return engine.IsReady(); });
  ASSERT_TRUE(engine.IsReady());

  qtrade::sdk::trader::OrderRequest request;
  request.strategy_id = "test-strategy";
  request.client_order_id = 1001;
  request.instrument = "IF2506";
  request.price = 10.5;
  request.volume = 2;
  request.side = qtrade::sdk::trader::SideType::kBuy;
  ASSERT_EQ(engine.GetOrderPipeline().Submit(request), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] {
    const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
    return order.has_value() && order->status == qtrade::sdk::trader::OrderStatusType::kFilled;
  });

  const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->traded_volume, 2);
  EXPECT_EQ(order->left_volume, 0);
  EXPECT_DOUBLE_EQ(engine.GetAccountManager().GetFilledAmount(), 21.0);
  EXPECT_EQ(engine.GetPositionManager().GetNetPosition("IF2506"), 2);

  qtrade::sdk::trader::OrderRequest invalid_request;
  invalid_request.strategy_id = "test-strategy";
  invalid_request.client_order_id = 1002;
  invalid_request.volume = 1;
  EXPECT_NE(engine.GetOrderPipeline().Submit(invalid_request), qtrade::ErrorCode::kSuccess);

  EXPECT_EQ(engine.Stop(), qtrade::ErrorCode::kSuccess);
}

TEST(OrderPipeline, CancelFlowsThroughEmsAndVenueReport) {
  const std::string suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  qtrade::engine::TradingEngine engine;
  ASSERT_EQ(engine.Init(MakeTestEngineConfig("cancel-engine-" + suffix)), qtrade::ErrorCode::kSuccess);

  qtrade::strategy::StrategyConfig strategy_config;
  strategy_config.enabled = true;
  strategy_config.strategy_id = "cancel-strategy";
  strategy_config.instruments = {"IF2506"};
  ASSERT_EQ(engine.AddStrategy(strategy_config, ""), qtrade::ErrorCode::kSuccess);

  InstallConnectedStubQuote(engine);
  auto trader_api = std::make_unique<qtrade::test::stub::StubTraderApi>();
  trader_api->SetAutoFill(false);
  auto* raw_trader_api = trader_api.get();
  engine.SetTraderApi(std::move(trader_api));
  qtrade::sdk::trader::ConnectRequest connect_request;
  connect_request.connection_string = "stub://cancel";
  ASSERT_EQ(raw_trader_api->Connect(connect_request), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  engine.SubscribeQuote({"IF2506"});
  WaitUntil([&] { return engine.IsReady(); });
  ASSERT_TRUE(engine.IsReady());

  qtrade::sdk::trader::OrderRequest request;
  request.strategy_id = "cancel-strategy";
  request.client_order_id = 2001;
  request.instrument = "IF2506";
  request.price = 10.5;
  request.volume = 2;
  ASSERT_EQ(engine.GetOrderPipeline().Submit(request), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] {
    const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
    return order.has_value() && order->broker_order_id != 0 &&
           engine.GetOrderApi().GetLifecycleState(order->order_id) ==
             qtrade::engine::oms::OrderLifecycleState::kWorking;
  });
  const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
  ASSERT_TRUE(order.has_value());
  ASSERT_NE(order->broker_order_id, 0U);
  ASSERT_EQ(engine.GetOrderPipeline().Cancel(order->order_id), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] {
    return engine.GetOrderApi().GetLifecycleState(order->order_id) ==
           qtrade::engine::oms::OrderLifecycleState::kCanceled;
  });
  EXPECT_EQ(engine.GetOrderApi().GetLifecycleState(order->order_id),
            qtrade::engine::oms::OrderLifecycleState::kCanceled);

  EXPECT_EQ(engine.Stop(), qtrade::ErrorCode::kSuccess);
}
