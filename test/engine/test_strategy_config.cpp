#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/strategy/strategy_manager.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace {

struct StrategyState {
  int starts = 0;
  int stops = 0;
  qtrade::strategy::StrategyConfig config;
};

class ConfigurableStrategy final : public qtrade::strategy::IStrategy {
 public:
  explicit ConfigurableStrategy(std::shared_ptr<StrategyState> state) : state_(std::move(state)) {}

  qtrade::ErrorCode Init(const qtrade::strategy::StrategyConfig& config) override {
    state_->config = config;
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode Start() override {
    ++state_->starts;
    return qtrade::ErrorCode::kSuccess;
  }

  void Stop() override {
    ++state_->stops;
  }

  void SetOrderSender(qtrade::strategy::OrderSender) override {}

  qtrade::strategy::StrategyConfig GetStrategyConfig() const override {
    return state_->config;
  }

  void OnTick(const qtrade_sdk::quote::MarketTick&) override {}
  void OnBar(const qtrade_sdk::quote::Bar&) override {}
  void OnOrder(const qtrade_sdk::trader::Order&) override {}
  void OnTrade(const qtrade_sdk::trader::Trade&) override {}

 private:
  std::shared_ptr<StrategyState> state_;
};

/// 单测用：非插件路径，删除器用 delete
[[nodiscard]] qtrade::engine::strategy::StrategyPtr MakeTestStrategyPtr(
  std::unique_ptr<qtrade::strategy::IStrategy> strategy) {
  return qtrade::engine::strategy::StrategyPtr{strategy.release(),
                                               [](qtrade::strategy::IStrategy* p) { delete p; }};
}

}  // namespace

TEST(StrategyConfig, RegistersAndStartsStrategy) {
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::strategy::StrategyManager manager(lanes);
  const auto state = std::make_shared<StrategyState>();
  auto strategy = MakeTestStrategyPtr(std::make_unique<ConfigurableStrategy>(state));

  qtrade::strategy::StrategyConfig config;
  config.strategy_id = "strategy-1";
  config.strategy_name = "configurable";
  config.enabled = true;
  config.instruments = {"IF2506"};
  config.order_volume = 1;
  config.order_threshold = 0.02;
  ASSERT_EQ(strategy->Init(config), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(manager.RegisterStrategy("strategy-1", std::move(strategy), {"IF2506"}), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(state->config.strategy_id, "strategy-1");
  EXPECT_DOUBLE_EQ(state->config.order_threshold.value_or(0.0), 0.02);

  manager.Start();
  EXPECT_EQ(state->starts, 1);

  manager.Stop();
  EXPECT_EQ(state->stops, 1);
}

TEST(StrategyConfig, RejectsDuplicateInstrumentRoutes) {
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::strategy::StrategyManager manager(lanes);
  ASSERT_EQ(manager.RegisterStrategy(
              "strategy-1",
              MakeTestStrategyPtr(std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IF2506"}),
            qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(manager.RegisterStrategy(
              "strategy-2",
              MakeTestStrategyPtr(std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IF2506"}),
            qtrade::ErrorCode::kSystemError);
}

TEST(StrategyConfig, RejectsRegisterWhileRunning) {
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::strategy::StrategyManager manager(lanes);
  ASSERT_EQ(manager.RegisterStrategy(
              "strategy-1",
              MakeTestStrategyPtr(std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IF2506"}),
            qtrade::ErrorCode::kSuccess);
  manager.Start();
  EXPECT_EQ(manager.RegisterStrategy(
              "strategy-2",
              MakeTestStrategyPtr(std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IC2506"}),
            qtrade::ErrorCode::kSystemError);
  manager.Stop();
}
