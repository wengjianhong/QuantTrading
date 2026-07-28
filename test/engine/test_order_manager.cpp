#include "qtrade/engine/oms/order_manager.hpp"

#include <gtest/gtest.h>

namespace {

qtrade::engine::oms::OrderManagerOptions MakeOptions() {
  qtrade::engine::oms::OrderManagerOptions options;
  options.tenant_id = "tenant";
  options.engine_id = "engine";
  options.engine_epoch = 7;
  return options;
}

}  // namespace

TEST(OrderManager, TracksLifecycleInMemory) {
  qtrade::engine::oms::OrderManager manager;
  ASSERT_EQ(manager.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);

  qtrade_sdk::trader::OrderRequest request;
  request.client_order_id = 42;
  request.instrument = "IF2506";
  request.price = 100.5;
  request.volume = 2;
  const auto order = manager.CreateOrder(request);
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->order_id.find("tenant-engine-7-"), 0U);

  ASSERT_EQ(manager.MarkEmsQueued(order->order_id), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(manager.MarkSendPending(order->order_id), qtrade::ErrorCode::kSuccess);

  qtrade_sdk::trader::Trade trade;
  trade.trade_id = "trade-1";
  trade.order_id = order->order_id;
  trade.client_order_id = request.client_order_id;
  trade.instrument = request.instrument;
  trade.price = request.price;
  trade.volume = request.volume;
  trade.trade_amount = request.price * static_cast<double>(request.volume);
  manager.ApplyTradeReport(trade);

  EXPECT_EQ(manager.GetLifecycleState(order->order_id), qtrade::engine::oms::OrderLifecycleState::kFilled);
  manager.Shutdown();
}

TEST(OrderManager, ExposesUncertainOrdersForReconciliation) {
  qtrade::engine::oms::OrderManager manager;
  ASSERT_EQ(manager.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);

  qtrade_sdk::trader::OrderRequest request;
  request.client_order_id = 43;
  request.instrument = "IF2506";
  request.price = 100.5;
  request.volume = 1;
  const auto order = manager.CreateOrder(request);
  ASSERT_TRUE(order.has_value());
  ASSERT_EQ(manager.MarkEmsQueued(order->order_id), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(manager.MarkSendPending(order->order_id), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(manager.RecordSendResult(order->order_id, qtrade::ErrorCode::kTimeout), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(manager.GetLifecycleState(order->order_id), qtrade::engine::oms::OrderLifecycleState::kSendUnknown);

  const auto uncertain = manager.GetOrdersRequiringReconciliation();
  ASSERT_EQ(uncertain.size(), 1U);
  EXPECT_EQ(uncertain.front().order_id, order->order_id);
  manager.Shutdown();
}

TEST(OrderManager, AdoptsBrokerOrderWithoutResubmit) {
  qtrade::engine::oms::OrderManager manager;
  ASSERT_EQ(manager.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);

  qtrade_sdk::trader::Order report;
  report.order_id = "broker-order-1";
  report.client_order_id = 99;
  report.instrument = "IF2506";
  report.price = 101.0;
  report.volume = 3;
  report.left_volume = 3;
  report.status = qtrade_sdk::trader::OrderStatusType::kNotTradedQueueing;
  manager.ReconcileBrokerOrder(report);

  const auto local = manager.GetOrder("broker-order-1");
  ASSERT_TRUE(local.has_value());
  EXPECT_EQ(local->left_volume, 3);
  EXPECT_EQ(manager.GetLifecycleState("broker-order-1"), qtrade::engine::oms::OrderLifecycleState::kWorking);
  EXPECT_TRUE(manager.GetOrdersRequiringReconciliation().empty());
  manager.Shutdown();
}

TEST(OrderManager, InitializeClearsPreviousMemory) {
  qtrade::engine::oms::OrderManager manager;
  ASSERT_EQ(manager.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);

  qtrade_sdk::trader::OrderRequest request;
  request.client_order_id = 1;
  request.instrument = "IF2506";
  request.price = 1.0;
  request.volume = 1;
  ASSERT_TRUE(manager.CreateOrder(request).has_value());
  manager.Shutdown();

  ASSERT_EQ(manager.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);
  EXPECT_FALSE(manager.GetOrderByClientId(1).has_value());
  manager.Shutdown();
}
