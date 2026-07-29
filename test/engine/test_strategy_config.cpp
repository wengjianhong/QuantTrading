#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/strategy/strategy_manager.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace {

struct StrategyState {
  int starts = 0;
  int pauses = 0;
  int resumes = 0;
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

  void Pause() override {
    ++state_->pauses;
  }

  void Resume() override {
    ++state_->resumes;
  }

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

TEST(StrategyConfig, CreatesAndControlsConfiguredStrategy) {
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::strategy::StrategyManager engine(lanes);
  const auto state = std::make_shared<StrategyState>();
  ASSERT_EQ(engine.RegisterFactory("test", [state] { return std::make_unique<ConfigurableStrategy>(state); }),
            qtrade::ErrorCode::kSuccess);

  qtrade::engine::strategy::StrategyRuntimeConfig config;
  config.strategy_id = "strategy-1";
  config.plugin = "test";
  config.enabled = true;
  config.instruments = {"IF2506"};
  config.params["threshold"] = "12";
  ASSERT_EQ(engine.ApplyConfiguration({config}), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(state->params.at("threshold"), "12");

  engine.Start();
  EXPECT_EQ(state->starts, 1);

  config.enabled = false;
  ASSERT_EQ(engine.ApplyConfiguration({config}), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(state->pauses, 1);

  config.enabled = true;
  config.params["threshold"] = "15";
  ASSERT_EQ(engine.ApplyConfiguration({config}), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(state->resumes, 1);
  EXPECT_EQ(state->params.at("threshold"), "15");

  engine.Stop();
  EXPECT_EQ(state->stops, 1);
}

TEST(StrategyConfig, RejectsDuplicateInstrumentRoutes) {
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::strategy::StrategyManager engine(lanes);
  ASSERT_EQ(engine.RegisterFactory(
              "test", [] { return std::make_unique<ConfigurableStrategy>(std::make_shared<StrategyState>()); }),
            qtrade::ErrorCode::kSuccess);

  qtrade::engine::strategy::StrategyRuntimeConfig first;
  first.strategy_id = "strategy-1";
  first.plugin = "test";
  first.enabled = true;
  first.instruments = {"IF2506"};
  auto second = first;
  second.strategy_id = "strategy-2";
  EXPECT_EQ(engine.ApplyConfiguration({first, second}), qtrade::ErrorCode::kSystemError);
}
