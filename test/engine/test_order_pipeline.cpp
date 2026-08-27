#include "qtrade/engine/compliance/compliance_api.hpp"
#include "qtrade/engine/core/order_intent_queue.hpp"
#include "qtrade/engine/core/order_pipeline.hpp"
#include "qtrade/engine/instance_risk/instance_risk_manager.hpp"

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

class DenyingCompliance final : public qtrade::engine::compliance::ComplianceApi {
 public:
  qtrade::ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest&) const override {
    return qtrade::ErrorCode::kPermissionDenied;
  }
};

class PassingCompliance final : public qtrade::engine::compliance::ComplianceApi {
 public:
  qtrade::ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest&) const override {
    return qtrade::ErrorCode::kSuccess;
  }
};

class PassingStrategyRisk final : public qtrade::engine::strategy_risk::StrategyRiskApi {
 public:
  qtrade::ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest&) const override {
    return qtrade::ErrorCode::kSuccess;
  }
};

class PassingInstanceRisk final : public qtrade::engine::instance_risk::InstanceRiskApi {
 public:
  std::uint64_t Version() const override {
    return 0;
  }

  qtrade::ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest&) const override {
    return qtrade::ErrorCode::kSuccess;
  }
};

class RecordingOrderApi final : public qtrade::engine::orders::OrderApi {
 public:
  std::string AllocateOrderId() override {
    allocate_called.store(true, std::memory_order_release);
    return "order-1";
  }

  std::optional<qtrade::sdk::trader::Order> CreateOrder(const qtrade::sdk::trader::OrderRequest& request,
                                                        const std::string& order_id) override {
    qtrade::sdk::trader::Order order;
    order.order_id = order_id;
    order.instrument = request.instrument;
    return order;
  }

  std::optional<qtrade::sdk::trader::Order> GetOrderByClientId(std::uint32_t) const override {
    return std::nullopt;
  }

  std::optional<qtrade::sdk::trader::Order> GetOrder(const std::string&) const override {
    return std::nullopt;
  }

  std::optional<qtrade::engine::orders::OrderLifecycleState> GetLifecycleState(const std::string&) const override {
    return qtrade::engine::orders::OrderLifecycleState::kPrepared;
  }

  qtrade::ErrorCode CancelOrder(const std::string&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode MarkEmsQueued(const std::string&) override {
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

  std::atomic<bool> allocate_called{false};
};

class RecordingExecutionApi final : public qtrade::engine::execution::ExecutionApi {
 public:
  qtrade::ErrorCode Enqueue(const qtrade::sdk::trader::Order&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode EnqueueCancel(const qtrade::sdk::trader::CancelOrderRequest&) override {
    return qtrade::ErrorCode::kSuccess;
  }
};

class RecordingAccountRiskApi final : public qtrade::engine::account_risk::AccountRiskApi {
 public:
  qtrade::ErrorCode Reserve(const qtrade::sdk::trader::OrderRequest&, const std::string&) override {
    in_reserve.store(true, std::memory_order_release);
    std::this_thread::sleep_for(hold);
    reserve_called.store(true, std::memory_order_release);
    in_reserve.store(false, std::memory_order_release);
    return qtrade::ErrorCode::kSuccess;
  }

  void Release(std::string, qtrade::account_risk::ReleaseReason) override {}

  std::chrono::milliseconds hold{0};
  std::atomic<bool> in_reserve{false};
  std::atomic<bool> reserve_called{false};
};

}  // namespace

TEST(OrderPipeline, ComplianceDenialPrecedesOmsAndAccountReservation) {
  DenyingCompliance compliance;
  PassingStrategyRisk strategy_risk;
  PassingInstanceRisk instance_risk;
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  RecordingAccountRiskApi account_risk;
  qtrade::engine::OrderIntentQueue intent_queue{account_risk, orders, execution};
  qtrade::engine::OrderPipeline pipeline{compliance, strategy_risk, instance_risk, orders, execution, intent_queue};

  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  EXPECT_EQ(pipeline.Submit(request), qtrade::ErrorCode::kPermissionDenied);
  EXPECT_FALSE(orders.allocate_called.load());
  EXPECT_FALSE(account_risk.reserve_called.load());
}

TEST(OrderPipeline, SubmitEnqueuesWithoutWaitingForReserve) {
  PassingCompliance compliance;
  PassingStrategyRisk strategy_risk;
  PassingInstanceRisk instance_risk;
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  RecordingAccountRiskApi account_risk;
  account_risk.hold = std::chrono::milliseconds(120);
  qtrade::engine::OrderIntentQueue intent_queue{account_risk, orders, execution};
  intent_queue.Start();
  qtrade::engine::OrderPipeline pipeline{compliance, strategy_risk, instance_risk, orders, execution, intent_queue};

  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  const auto started = std::chrono::steady_clock::now();
  EXPECT_EQ(pipeline.Submit(request), qtrade::ErrorCode::kSuccess);
  const auto elapsed = std::chrono::steady_clock::now() - started;
  EXPECT_LT(elapsed, std::chrono::milliseconds(50));
  EXPECT_FALSE(orders.allocate_called.load());

  WaitUntil([&] { return account_risk.reserve_called.load(std::memory_order_acquire); },
            std::chrono::milliseconds(500));
  EXPECT_TRUE(account_risk.reserve_called.load());
  EXPECT_TRUE(orders.allocate_called.load());

  intent_queue.Stop();
}

TEST(OrderPipeline, InstanceRiskSeesInflightIntent) {
  PassingCompliance compliance;
  PassingStrategyRisk strategy_risk;
  qtrade::engine::instance_risk::InstanceRiskManager instance_risk;
  qtrade::engine::instance_risk::InstanceRiskLimits limits;
  limits.max_open_orders = 1;
  ASSERT_EQ(instance_risk.Configure(limits), qtrade::ErrorCode::kSuccess);

  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  RecordingAccountRiskApi account_risk;
  account_risk.hold = std::chrono::milliseconds(120);
  qtrade::engine::OrderIntentQueue intent_queue{account_risk, orders, execution};
  instance_risk.SetStateProviders([&] { return intent_queue.PendingCount(); },
                                  [&] { return intent_queue.PendingNotional(); });
  intent_queue.Start();
  qtrade::engine::OrderPipeline pipeline{compliance, strategy_risk, instance_risk, orders, execution, intent_queue};

  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  request.price = 100.0;
  request.volume = 1;
  EXPECT_EQ(pipeline.Submit(request), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(pipeline.Submit(request), qtrade::ErrorCode::kResourceExhausted);

  intent_queue.Stop();
}
