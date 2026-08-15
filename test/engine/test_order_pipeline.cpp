#include "qtrade/engine/compliance/compliance_api.hpp"
#include "qtrade/engine/core/order_pipeline.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace {

class DenyingCompliance final : public qtrade::engine::compliance::ComplianceApi {
 public:
  qtrade::ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest&) const override {
    return qtrade::ErrorCode::kPermissionDenied;
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
    allocate_called = true;
    return "order-1";
  }

  std::optional<qtrade::sdk::trader::Order> CreateOrder(const qtrade::sdk::trader::OrderRequest&,
                                                         const std::string&) override {
    return std::nullopt;
  }

  std::optional<qtrade::sdk::trader::Order> GetOrderByClientId(std::uint32_t) const override {
    return std::nullopt;
  }

  std::optional<qtrade::sdk::trader::Order> GetOrder(const std::string&) const override {
    return std::nullopt;
  }

  std::optional<qtrade::engine::orders::OrderLifecycleState> GetLifecycleState(
    const std::string&) const override {
    return std::nullopt;
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

  bool allocate_called{false};
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
    reserve_called = true;
    return qtrade::ErrorCode::kSuccess;
  }

  void Release(std::string, qtrade::account_risk::ReleaseReason) override {}

  bool reserve_called{false};
};

}  // namespace

TEST(OrderPipeline, ComplianceDenialPrecedesOmsAndAccountReservation) {
  DenyingCompliance compliance;
  PassingStrategyRisk strategy_risk;
  PassingInstanceRisk instance_risk;
  RecordingOrderApi orders;
  RecordingExecutionApi execution;
  RecordingAccountRiskApi account_risk;
  qtrade::engine::OrderPipeline pipeline{
    compliance, strategy_risk, instance_risk, orders, execution, account_risk};

  qtrade::sdk::trader::OrderRequest request;
  request.instrument = "IF2506";
  EXPECT_EQ(pipeline.Submit(request), qtrade::ErrorCode::kPermissionDenied);
  EXPECT_FALSE(orders.allocate_called);
  EXPECT_FALSE(account_risk.reserve_called);
}
