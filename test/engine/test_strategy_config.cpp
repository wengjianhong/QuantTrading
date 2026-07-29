#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/strategy/strategy_manager.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace {

struct StrategyState {
  int starts = 0;
  int stops = 0;
  std::unordered_map<std::string, std::string> params;
};

class ConfigurableStrategy final : public qtrade::strategy::IStrategy {
 public:
  explicit ConfigurableStrategy(std::shared_ptr<StrategyState> state) : state_(std::move(state)) {}

  qtrade::ErrorCode Init(const qtrade::strategy::StrategyConfig&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  qtrade::ErrorCode Start() override {
    ++state_->starts;
    return qtrade::ErrorCode::kSuccess;
  }

  void Pause() override {}
  void Resume() override {}

  void Stop() override {
    ++state_->stops;
  }

  void OnTick(const qtrade_sdk::quote::MarketTick&) override {}
  void OnBar(const qtrade_sdk::quote::Bar&) override {}
  void OnOrder(const qtrade_sdk::trader::Order&) override {}
  void OnTrade(const qtrade_sdk::trader::Trade&) override {}

  qtrade::ErrorCode SendSignal(const qtrade::strategy::Signal&) override {
    return qtrade::ErrorCode::kSuccess;
  }

  std::string GetParameter(const std::string& key) const override {
    const auto it = state_->params.find(key);
    return it == state_->params.end() ? std::string{} : it->second;
  }

  qtrade::ErrorCode SetParameter(const std::string& key, const std::string& value) override {
    state_->params[key] = value;
    return qtrade::ErrorCode::kSuccess;
  }

 private:
  std::shared_ptr<StrategyState> state_;
};

}  // namespace

TEST(StrategyConfig, RegistersAndStartsStrategy) {
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::strategy::StrategyManager manager(lanes);
  const auto state = std::make_shared<StrategyState>();
  auto strategy = qtrade::engine::strategy::MakeStrategyPtr(std::make_unique<ConfigurableStrategy>(state));
  ASSERT_EQ(strategy->SetParameter("threshold", "12"), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(manager.RegisterStrategy("strategy-1", std::move(strategy), {"IF2506"}), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(state->params.at("threshold"), "12");

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
              qtrade::engine::strategy::MakeStrategyPtr(
                std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IF2506"}),
            qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(manager.RegisterStrategy(
              "strategy-2",
              qtrade::engine::strategy::MakeStrategyPtr(
                std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IF2506"}),
            qtrade::ErrorCode::kSystemError);
}

TEST(StrategyConfig, RejectsRegisterWhileRunning) {
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::strategy::StrategyManager manager(lanes);
  ASSERT_EQ(manager.RegisterStrategy(
              "strategy-1",
              qtrade::engine::strategy::MakeStrategyPtr(
                std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IF2506"}),
            qtrade::ErrorCode::kSuccess);
  manager.Start();
  EXPECT_EQ(manager.RegisterStrategy(
              "strategy-2",
              qtrade::engine::strategy::MakeStrategyPtr(
                std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>())),
              {"IC2506"}),
            qtrade::ErrorCode::kSystemError);
  manager.Stop();
}
