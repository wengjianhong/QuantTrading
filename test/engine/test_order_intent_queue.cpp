/// @file      test_order_intent_queue.cpp
/// @brief     OrderIntentQueue：A 段入队与 E 段预占线程隔离
#include "qtrade/engine/account_risk/account_risk_api.hpp"
#include "qtrade/engine/core/order_intent_queue.hpp"
#include "qtrade/engine/execution/execution_api.hpp"
#include "qtrade/engine/orders/order_api.hpp"
#include "qtrade/engine/orders/order_manager.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

qtrade::engine::orders::OrderManagerOptions MakeOptions() {
  qtrade::engine::orders::OrderManagerOptions options;
  options.account_id = "acct";
  options.engine_id = "engine";
  return options;
}

class RecordingOrderApi final : public qtrade::engine::orders::OrderApi {
 public:
  std::string AllocateOrderId() override {
    return "order-" + std::to_string(next_id.fetch_add(1, std::memory_order_relaxed) + 1);
  }

  std::optional<qtrade::sdk::trader::Order> CreateOrder(const qtrade::sdk::trader::OrderRequest& request,
                                                        const std::string& order_id) override {
    std::lock_guard lock(mutex);
    if (request.client_order_id != 0) {
      if (const auto it = by_client.find(request.client_order_id); it != by_client.end()) {
        return by_id.at(it->second);
      }
    }
    qtrade::sdk::trader::Order order;
    order.order_id = order_id;
    order.client_order_id = request.client_order_id;
    order.instrument = request.instrument;
    order.price = request.price;
    order.volume = request.volume;
    order.left_volume = request.volume;
    by_id[order_id] = order;
    lifecycle[order_id] = qtrade::engine::orders::OrderLifecycleState::kPrepared;
    if (request.client_order_id != 0) {
      by_client[request.client_order_id] = order_id;
    }
    created_count.fetch_add(1, std::memory_order_relaxed);
    created.store(true, std::memory_order_release);
    return order;
  }

  std::optional<qtrade::sdk::trader::Order> GetOrderByClientId(std::uint32_t client_order_id) const override {
    std::lock_guard lock(mutex);
    const auto it = by_client.find(client_order_id);
    if (it == by_client.end()) {
      return std::nullopt;
    }
    return by_id.at(it->second);
  }

  std::optional<qtrade::sdk::trader::Order> GetOrder(const std::string& order_id) const override {
    std::lock_guard lock(mutex);
    const auto it = by_id.find(order_id);
    return it == by_id.end() ? std::nullopt : std::optional<qtrade::sdk::trader::Order>(it->second);
  }

  std::optional<qtrade::engine::orders::OrderLifecycleState> GetLifecycleState(
    const std::string& order_id) const override {
    std::lock_guard lock(mutex);
    const auto it = lifecycle.find(order_id);
    return it == lifecycle.end() ? std::nullopt
                                 : std::optional<qtrade::engine::orders::OrderLifecycleState>(it->second);
  }

  qtrade::ErrorCode CancelOrder(const std::string&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode MarkEmsQueued(const std::string& order_id) override {
    std::lock_guard lock(mutex);
    lifecycle[order_id] = qtrade::engine::orders::OrderLifecycleState::kEmsQueued;
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

  void ApplyOrderReport(const qtrade::sdk::trader::Order& report) override {
    std::lock_guard lock(mutex);
    last_report = report;
    if (auto it = by_id.find(report.order_id); it != by_id.end()) {
      it->second = report;
    }
    if (report.status == qtrade::sdk::trader::OrderStatusType::kRejected) {
      lifecycle[report.order_id] = qtrade::engine::orders::OrderLifecycleState::kRejected;
    } else if (report.status == qtrade::sdk::trader::OrderStatusType::kUnknown) {
      lifecycle[report.order_id] = qtrade::engine::orders::OrderLifecycleState::kSendUnknown;
    }
  }

  void ApplyTradeReport(const qtrade::sdk::trader::Trade&) override {}

  mutable std::mutex mutex;
  std::unordered_map<std::string, qtrade::sdk::trader::Order> by_id;
  std::unordered_map<std::uint32_t, std::string> by_client;
  std::unordered_map<std::string, qtrade::engine::orders::OrderLifecycleState> lifecycle;
  qtrade::sdk::trader::Order last_report;
  std::atomic<int> next_id{0};
  std::atomic<int> created_count{0};
  std::atomic<bool> created{false};
  std::atomic<bool> ems_queued{false};
};

class RecordingExecutionApi final : public qtrade::engine::execution::ExecutionApi {
 public:
  qtrade::ErrorCode Enqueue(const qtrade::sdk::trader::Order&) override {
    enqueued_count.fetch_add(1, std::memory_order_relaxed);
    enqueued.store(true, std::memory_order_release);
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode EnqueueCancel(const qtrade::sdk::trader::CancelOrderRequest&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  std::atomic<bool> enqueued{false};
  std::atomic<int> enqueued_count{0};
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

  void Release(std::string order_id, qtrade::account_risk::ReleaseReason) override {
    released_count.fetch_add(1, std::memory_order_relaxed);
    last_released = std::move(order_id);
  }

  std::atomic<bool> in_reserve{false};
  std::atomic<int> reserve_calls{0};
  std::atomic<int> released_count{0};
  std::string last_released;
};

class FailingReserveApi final : public qtrade::engine::account_risk::AccountRiskApi {
 public:
  explicit FailingReserveApi(qtrade::ErrorCode code) : code(code) {}

  qtrade::ErrorCode Reserve(const qtrade::sdk::trader::OrderRequest&, const std::string&) override {
    reserve_calls.fetch_add(1, std::memory_order_relaxed);
    return code;
  }

  void Release(std::string, qtrade::account_risk::ReleaseReason) override {
    released.store(true, std::memory_order_release);
  }

  qtrade::ErrorCode code;
  std::atomic<int> reserve_calls{0};
  std::atomic<bool> released{false};
};

class ThrowingReserveApi final : public qtrade::engine::account_risk::AccountRiskApi {
 public:
  qtrade::ErrorCode Reserve(const qtrade::sdk::trader::OrderRequest&, const std::string&) override {
    const int n = reserve_calls.fetch_add(1, std::memory_order_relaxed);
    if (n == 0) {
      throw 42;
    }
    return qtrade::ErrorCode::kSuccess;
  }

  void Release(std::string, qtrade::account_risk::ReleaseReason) override {}

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
  EXPECT_EQ(queue.PendingCount(), 0U);

  queue.Stop();
}

TEST(OrderIntentQueue, RejectsWhenFull) {
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  SlowReserveApi account_risk;
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution, 1};
  queue.Start();

  qtrade::engine::OrderIntent first;
  first.request.instrument = "IF2506";
  ASSERT_EQ(queue.Enqueue(std::move(first)), qtrade::ErrorCode::kSuccess);
  WaitUntil([&] { return account_risk.in_reserve.load(std::memory_order_acquire); }, std::chrono::milliseconds(200));

  qtrade::engine::OrderIntent second;
  second.request.instrument = "IC2506";
  ASSERT_EQ(queue.Enqueue(std::move(second)), qtrade::ErrorCode::kSuccess);

  qtrade::engine::OrderIntent third;
  third.request.instrument = "IH2506";
  EXPECT_EQ(queue.Enqueue(std::move(third)), qtrade::ErrorCode::kResourceExhausted);

  queue.Stop();
}

TEST(OrderIntentQueue, CountsPendingWhileExecuting) {
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  SlowReserveApi account_risk;
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution};
  queue.Start();

  qtrade::engine::OrderIntent intent;
  intent.request.instrument = "IF2506";
  intent.request.price = 10.0;
  intent.request.volume = 2;
  ASSERT_EQ(queue.Enqueue(std::move(intent)), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(queue.PendingCount(), 1U);
  EXPECT_DOUBLE_EQ(queue.PendingNotional(), 20.0);

  WaitUntil([&] { return execution.enqueued.load(std::memory_order_acquire); }, std::chrono::milliseconds(500));
  WaitUntil([&] { return queue.PendingCount() == 0; }, std::chrono::milliseconds(200));
  EXPECT_EQ(queue.PendingCount(), 0U);
  EXPECT_DOUBLE_EQ(queue.PendingNotional(), 0.0);

  queue.Stop();
}

TEST(OrderIntentQueue, ReserveFailureCreatesRejectedOrderAndNotifies) {
  qtrade::engine::orders::OrderManager orders;
  ASSERT_EQ(orders.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);
  RecordingExecutionApi execution;
  FailingReserveApi account_risk{qtrade::ErrorCode::kPermissionDenied};
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution};

  std::mutex mu;
  std::vector<qtrade::sdk::trader::Order> outcomes;
  queue.SetOutcomeHandler([&](const qtrade::sdk::trader::Order& order) {
    std::lock_guard lock(mu);
    outcomes.push_back(order);
  });
  queue.Start();

  qtrade::engine::OrderIntent intent;
  intent.request.client_order_id = 11;
  intent.request.instrument = "IF2506";
  intent.request.price = 100.0;
  intent.request.volume = 1;
  ASSERT_EQ(queue.Enqueue(std::move(intent)), qtrade::ErrorCode::kSuccess);

  WaitUntil(
    [&] {
      std::lock_guard lock(mu);
      return !outcomes.empty();
    },
    std::chrono::milliseconds(500));

  {
    std::lock_guard lock(mu);
    ASSERT_EQ(outcomes.size(), 1U);
    EXPECT_EQ(outcomes.front().status, qtrade::sdk::trader::OrderStatusType::kRejected);
    EXPECT_EQ(outcomes.front().instrument, "IF2506");
  }
  const auto local = orders.GetOrderByClientId(11);
  ASSERT_TRUE(local.has_value());
  EXPECT_EQ(orders.GetLifecycleState(local->order_id), qtrade::engine::orders::OrderLifecycleState::kRejected);
  EXPECT_FALSE(execution.enqueued.load());
  EXPECT_FALSE(account_risk.released.load());

  queue.Stop();
  orders.Shutdown();
}

TEST(OrderIntentQueue, ReserveTimeoutCreatesUnknownOrderAndNotifies) {
  qtrade::engine::orders::OrderManager orders;
  ASSERT_EQ(orders.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);
  RecordingExecutionApi execution;
  FailingReserveApi account_risk{qtrade::ErrorCode::kTimeout};
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution};

  std::atomic<bool> notified{false};
  qtrade::sdk::trader::OrderStatusType status = qtrade::sdk::trader::OrderStatusType::kInit;
  queue.SetOutcomeHandler([&](const qtrade::sdk::trader::Order& order) {
    status = order.status;
    notified.store(true, std::memory_order_release);
  });
  queue.Start();

  qtrade::engine::OrderIntent intent;
  intent.request.client_order_id = 12;
  intent.request.instrument = "IF2506";
  intent.request.price = 100.0;
  intent.request.volume = 1;
  ASSERT_EQ(queue.Enqueue(std::move(intent)), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] { return notified.load(std::memory_order_acquire); }, std::chrono::milliseconds(500));
  EXPECT_EQ(status, qtrade::sdk::trader::OrderStatusType::kUnknown);
  const auto local = orders.GetOrderByClientId(12);
  ASSERT_TRUE(local.has_value());
  EXPECT_EQ(orders.GetLifecycleState(local->order_id), qtrade::engine::orders::OrderLifecycleState::kSendUnknown);
  EXPECT_FALSE(execution.enqueued.load());
  EXPECT_FALSE(account_risk.released.load());

  queue.Stop();
  orders.Shutdown();
}

TEST(OrderIntentQueue, DuplicateClientOrderIdReservesOnce) {
  qtrade::engine::orders::OrderManager orders;
  ASSERT_EQ(orders.Initialize(MakeOptions()), qtrade::ErrorCode::kSuccess);
  RecordingExecutionApi execution;
  SlowReserveApi account_risk;
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution};
  queue.Start();

  qtrade::engine::OrderIntent first;
  first.request.client_order_id = 7;
  first.request.instrument = "IF2506";
  first.request.price = 1.0;
  first.request.volume = 1;
  qtrade::engine::OrderIntent second = first;
  ASSERT_EQ(queue.Enqueue(std::move(first)), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(queue.Enqueue(std::move(second)), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] { return execution.enqueued.load(std::memory_order_acquire); }, std::chrono::milliseconds(500));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(account_risk.reserve_calls.load(), 1);
  EXPECT_EQ(execution.enqueued_count.load(), 1);
  EXPECT_EQ(orders.GetActiveOrderCount(), 1U);

  queue.Stop();
  orders.Shutdown();
}

TEST(OrderIntentQueue, ExecuteExceptionDoesNotStopWorker) {
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  ThrowingReserveApi account_risk;
  qtrade::engine::OrderIntentQueue queue{account_risk, orders, execution};
  queue.Start();

  qtrade::engine::OrderIntent first;
  first.request.instrument = "IF2506";
  qtrade::engine::OrderIntent second;
  second.request.instrument = "IC2506";
  ASSERT_EQ(queue.Enqueue(std::move(first)), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(queue.Enqueue(std::move(second)), qtrade::ErrorCode::kSuccess);

  WaitUntil([&] { return execution.enqueued.load(std::memory_order_acquire); }, std::chrono::milliseconds(500));
  EXPECT_GE(account_risk.reserve_calls.load(), 2);
  EXPECT_TRUE(execution.enqueued.load());

  queue.Stop();
}
