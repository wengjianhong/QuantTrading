#include "qtrade/common/config/qtrade_engine_config.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "qtrade_sdk/mock/trader/mock_trader_api.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <functional>
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

}  // namespace

TEST(OrderPipeline, MockOrderFlowsThroughOmsAndReturnLane) {
  qtrade::engine::TradingEngine engine;
  qtrade::common::config::QtradeEngineConfig config;
  config.identity.tenant_id = "test";
  config.identity.engine_id = "test-engine";
  config.identity.account_id = "test-account";
  config.support_services.config_service.enabled = false;
  config.support_services.account_risk_service.enabled = false;
  config.support_services.log_service.extensions["topic"] = "test";
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);

  auto& trader_normalizer = engine.GetTraderNormalizer();
  trader_normalizer.SetTraderApi(qtrade::adapter::mock::trader::CreateMockTraderApi());
  qtrade_sdk::trader::ConnectRequest connect_request;
  connect_request.connection_string = "mock://test";
  ASSERT_EQ(trader_normalizer.GetTraderApi()->Connect(connect_request), qtrade::ErrorCode::kSuccess);

  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);

  qtrade_sdk::trader::OrderRequest request;
  request.client_order_id = 1001;
  request.instrument = "IF2506";
  request.price = 10.5;
  request.volume = 2;
  request.side = qtrade_sdk::trader::SideType::kBuy;
  ASSERT_EQ(engine.SubmitOrder(request), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] {
    const auto order = engine.GetOrderManager().GetOrderByClientId(request.client_order_id);
    return order.has_value() && order->status == qtrade_sdk::trader::OrderStatusType::kFilled;
  });

  const auto order = engine.GetOrderManager().GetOrderByClientId(request.client_order_id);
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
