#include "qtrade/engine/oms/order_manager.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace {

std::string TemporaryJournalPath() {
  const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
  return (std::filesystem::temp_directory_path() / ("qtrade-order-journal-" + std::to_string(suffix) + ".jsonl"))
    .string();
}

qtrade::engine::oms::OrderManagerOptions MakeOptions(const std::string& path) {
  qtrade::engine::oms::OrderManagerOptions options;
  options.tenant_id = "tenant";
  options.engine_id = "engine";
  options.engine_epoch = 7;
  options.journal_path = path;
  options.fsync_on_append = true;
  return options;
}

}  // namespace

TEST(OrderJournal, RecoversCommittedOrderState) {
  const std::string path = TemporaryJournalPath();
  std::string order_id;

  {
    qtrade::engine::oms::OrderManager manager;
    ASSERT_EQ(manager.Initialize(MakeOptions(path)), qtrade::ErrorCode::kSuccess);

    qtrade_sdk::trader::OrderRequest request;
    request.client_order_id = 42;
    request.instrument = "IF2506";
    request.price = 100.5;
    request.volume = 2;
    const auto order = manager.CreateOrder(request);
    ASSERT_TRUE(order.has_value());
    order_id = order->order_id;
    EXPECT_EQ(order_id.find("tenant-engine-7-"), 0U);

    ASSERT_EQ(manager.MarkEmsQueued(order_id), qtrade::ErrorCode::kSuccess);
    ASSERT_EQ(manager.MarkSendPending(order_id), qtrade::ErrorCode::kSuccess);

    qtrade_sdk::trader::Trade trade;
    trade.trade_id = "trade-1";
    trade.order_id = order_id;
    trade.client_order_id = request.client_order_id;
    trade.instrument = request.instrument;
    trade.price = request.price;
    trade.volume = request.volume;
    trade.trade_amount = request.price * static_cast<double>(request.volume);
    manager.ApplyTradeReport(trade);

    EXPECT_EQ(manager.GetLifecycleState(order_id), qtrade::engine::oms::OrderLifecycleState::kFilled);
    manager.Shutdown();
  }

  {
    qtrade::engine::oms::OrderManager recovered;
    ASSERT_EQ(recovered.Initialize(MakeOptions(path)), qtrade::ErrorCode::kSuccess);
    const auto order = recovered.GetOrder(order_id);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->traded_volume, 2);
    EXPECT_DOUBLE_EQ(order->trade_amount, 201.0);
    EXPECT_EQ(recovered.GetLifecycleState(order_id), qtrade::engine::oms::OrderLifecycleState::kFilled);
    EXPECT_TRUE(recovered.GetOrdersRequiringReconciliation().empty());
    recovered.Shutdown();
  }

  std::filesystem::remove(path);
}

TEST(OrderJournal, ExposesUncertainOrdersForReconciliation) {
  const std::string path = TemporaryJournalPath();
  {
    qtrade::engine::oms::OrderManager manager;
    ASSERT_EQ(manager.Initialize(MakeOptions(path)), qtrade::ErrorCode::kSuccess);

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
    manager.Shutdown();
  }

  {
    qtrade::engine::oms::OrderManager recovered;
    ASSERT_EQ(recovered.Initialize(MakeOptions(path)), qtrade::ErrorCode::kSuccess);
    const auto uncertain = recovered.GetOrdersRequiringReconciliation();
    ASSERT_EQ(uncertain.size(), 1U);
    EXPECT_EQ(recovered.GetLifecycleState(uncertain.front().order_id),
              qtrade::engine::oms::OrderLifecycleState::kSendUnknown);
    recovered.Shutdown();
  }

  std::filesystem::remove(path);
}
