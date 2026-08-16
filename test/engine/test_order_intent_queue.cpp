/// @file      test_order_intent_queue.cpp
/// @brief     OrderIntentQueue：A 段入队与 E 段预占线程隔离
#include "qtrade/engine/account_risk/account_risk_api.hpp"
#include "qtrade/engine/core/order_intent_queue.hpp"
#include "qtrade/engine/execution/execution_api.hpp"
#include "qtrade/engine/orders/order_api.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <thread>

namespace {

void WaitUntil(const std::function<bool()>& pred, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

class RecordingOrderApi final : public qtrade::engine::orders::OrderApi {
 public:
  std::string AllocateOrderId() override {
    return "order-1";
  }

  std::optional<qtrade::sdk::trader::Order> CreateOrder(const qtrade::sdk::trader::OrderRequest& request,
                                                         const std::string& order_id) override {
    qtrade::sdk::trader::Order order;
    order.order_id = order_id;
    order.instrument = request.instrument;
    created.store(true, std::memory_order_release);
    return order;
  }

  std::optional<qtrade::sdk::trader::Order> GetOrderByClientId(std::uint32_t) const override {
    return std::nullopt;
  }

  std::optional<qtrade::sdk::trader::Order> GetOrder(const std::string&) const override {
    return std::nullopt;
  }

  std::optional<qtrade::engine::orders::OrderLifecycleState> GetLifecycleState(
    const std::string&) const override {
    return qtrade::engine::orders::OrderLifecycleState::kPrepared;
  }

  qtrade::ErrorCode CancelOrder(const std::string&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode MarkEmsQueued(const std::string&) override {
    ems_queued.store(true, std::memory_order_release);
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode MarkSendPending(const std::string&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode RecordSendResult(const std::string&, qtrade::ErrorCode) override {
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode RecordCancelResult(const std::string&, qtrade::ErrorCode) override {
    return qtrade::ErrorCode::kSuccess;
  }

  void ApplyOrderReport(const qtrade::sdk::trader::Order&) override {}
  void ApplyTradeReport(const qtrade::sdk::trader::Trade&) override {}

  std::atomic<bool> created{false};
  std::atomic<bool> ems_queued{false};
};

class RecordingExecutionApi final : public qtrade::engine::execution::ExecutionApi {
 public:
  qtrade::ErrorCode Enqueue(const qtrade::sdk::trader::Order&) override {
    enqueued.store(true, std::memory_order_release);
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode EnqueueCancel(const qtrade::sdk::trader::CancelOrderRequest&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  std::atomic<bool> enqueued{false};
};

class SlowReserveApi final : public qtrade::engine::account_risk::AccountRiskApi {
 public:
  qtrade::ErrorCode Reserve(const qtrade::sdk::trader::OrderRequest&, const std::string&) override {
    in_reserve.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    reserve_calls.fetch_add(1, std::memory_order_relaxed);
    in_reserve.store(false, std::memory_order_release);
    return qtrade::ErrorCode::kSuccess;
  }

  void Release(std::string, qtrade::account_risk::ReleaseReason) override {}

  std::atomic<bool> in_reserve{false};
  std::atomic<int> reserve_calls{0};
};

}  // namespace

TEST(OrderIntentQueue, RejectsEnqueueBeforeStart) {
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  SlowReserveApi account_risk;
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution};

  qtrade::engine::OrderIntent intent;
  intent.request.instrument = "IF2506";
  EXPECT_EQ(queue.Enqueue(std::move(intent)), qtrade::ErrorCode::kNotInitialized);
  EXPECT_EQ(account_risk.reserve_calls.load(), 0);
}

TEST(OrderIntentQueue, EnqueueReturnsBeforeReserveFinishes) {
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  SlowReserveApi account_risk;
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution};
  queue.Start();

  qtrade::engine::OrderIntent intent;
  intent.request.instrument = "IF2506";
  const auto started = std::chrono::steady_clock::now();
  EXPECT_EQ(queue.Enqueue(std::move(intent)), qtrade::ErrorCode::kSuccess);
  const auto elapsed = std::chrono::steady_clock::now() - started;
  EXPECT_LT(elapsed, std::chrono::milliseconds(50));

  WaitUntil([&] { return orders.created.load(std::memory_order_acquire); }, std::chrono::milliseconds(500));
  EXPECT_EQ(account_risk.reserve_calls.load(), 1);
  EXPECT_TRUE(orders.created.load());
  EXPECT_TRUE(execution.enqueued.load());

  queue.Stop();
}
