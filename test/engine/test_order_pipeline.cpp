#include "qtrade/common/config/qtrade_engine_config.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "qtrade_sdk/mock/quote/mock_quote_api.hpp"
#include "qtrade_sdk/mock/trader/mock_trader_api.hpp"

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

void InstallConnectedMockQuote(qtrade::engine::TradingEngine& engine) {
  auto quote_api = qtrade::adapter::mock::quote::CreateMockQuoteApi();
  qtrade_sdk::quote::ConnectRequest request;
  request.connection_string = "mock://quote";
  EXPECT_EQ(quote_api->Connect(request), qtrade::ErrorCode::kSuccess);
  engine.SetQuoteApi(std::move(quote_api));
}

}  // namespace

TEST(OrderPipeline, MockOrderFlowsThroughOmsAndTraderLane) {
  const std::string suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  qtrade::engine::TradingEngine engine;
  qtrade::common::config::QtradeEngineConfig config;
  config.identity.tenant_id = "test";
  config.identity.engine_id = "test-engine-" + suffix;
  config.identity.account_id = "test-account";
  config.support_services.config_service.enabled = false;
  config.support_services.account_risk_service.enabled = false;
  config.support_services.log_service.extensions["topic"] = "test";
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);

  InstallConnectedMockQuote(engine);
  engine.SetTraderApi(qtrade::adapter::mock::trader::CreateMockTraderApi());
  qtrade_sdk::trader::ConnectRequest connect_request;
  connect_request.connection_string = "mock://test";
  ASSERT_EQ(engine.GetTraderApi()->Connect(connect_request), qtrade::ErrorCode::kSuccess);

  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  engine.SubscribeQuote({"IF2506"});
  WaitUntil([&] { return engine.IsReady(); });
  ASSERT_TRUE(engine.IsReady());

  qtrade_sdk::trader::OrderRequest request;
  request.client_order_id = 1001;
  request.instrument = "IF2506";
  request.price = 10.5;
  request.volume = 2;
  request.side = qtrade_sdk::trader::SideType::kBuy;
  ASSERT_EQ(engine.SubmitOrder(request), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] {
    const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
    return order.has_value() && order->status == qtrade_sdk::trader::OrderStatusType::kFilled;
  });

  const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->traded_volume, 2);
  EXPECT_EQ(order->left_volume, 0);
  EXPECT_DOUBLE_EQ(engine.GetAccountManager().GetFilledAmount(), 21.0);
  EXPECT_EQ(engine.GetPositionManager().GetNetPosition("IF2506"), 2);

  qtrade_sdk::trader::OrderRequest invalid_request;
  invalid_request.client_order_id = 1002;
  invalid_request.volume = 1;
  EXPECT_NE(engine.SubmitOrder(invalid_request), qtrade::ErrorCode::kSuccess);

  EXPECT_EQ(engine.Stop(), qtrade::ErrorCode::kSuccess);
}

TEST(OrderPipeline, CancelFlowsThroughEmsAndVenueReport) {
  const std::string suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  qtrade::engine::TradingEngine engine;
  qtrade::common::config::QtradeEngineConfig config;
  config.identity.tenant_id = "test";
  config.identity.engine_id = "cancel-engine-" + suffix;
  config.identity.account_id = "test-account";
  config.support_services.config_service.enabled = false;
  config.support_services.account_risk_service.enabled = false;
  config.support_services.log_service.extensions["topic"] = "test";
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);

  InstallConnectedMockQuote(engine);
  auto trader_api = std::make_unique<qtrade::adapter::mock::trader::MockTraderApi>();
  trader_api->SetAutoFill(false);
  auto* raw_trader_api = trader_api.get();
  engine.SetTraderApi(std::move(trader_api));
  qtrade_sdk::trader::ConnectRequest connect_request;
  connect_request.connection_string = "mock://cancel";
  ASSERT_EQ(raw_trader_api->Connect(connect_request), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  engine.SubscribeQuote({"IF2506"});
  WaitUntil([&] { return engine.IsReady(); });
  ASSERT_TRUE(engine.IsReady());

  qtrade_sdk::trader::OrderRequest request;
  request.client_order_id = 2001;
  request.instrument = "IF2506";
  request.price = 10.5;
  request.volume = 2;
  ASSERT_EQ(engine.SubmitOrder(request), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] {
    const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
    return order.has_value() && order->order_emt_id != 0 &&
           engine.GetOrderApi().GetLifecycleState(order->order_id) ==
             qtrade::engine::oms::OrderLifecycleState::kWorking;
  });
  const auto order = engine.GetOrderApi().GetOrderByClientId(request.client_order_id);
  ASSERT_TRUE(order.has_value());
  ASSERT_NE(order->order_emt_id, 0U);
  ASSERT_EQ(engine.CancelOrder(order->order_id), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] {
    return engine.GetOrderApi().GetLifecycleState(order->order_id) ==
           qtrade::engine::oms::OrderLifecycleState::kCanceled;
  });
  EXPECT_EQ(engine.GetOrderApi().GetLifecycleState(order->order_id),
            qtrade::engine::oms::OrderLifecycleState::kCanceled);

  EXPECT_EQ(engine.Stop(), qtrade::ErrorCode::kSuccess);
}
