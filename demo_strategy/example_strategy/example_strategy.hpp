/// @file      example_strategy.hpp
/// @brief     示例策略
/// @details   实现一个简单的趋势跟踪策略作为示例
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADE_DEMO_EXAMPLE_STRATEGY_HPP
#define QTRADE_TRADE_DEMO_EXAMPLE_STRATEGY_HPP
#include <qtrade/strategy/strategy.hpp>

#include <memory>

namespace qtrade::demo {

/// @brief 简单的趋势跟踪策略
class ExampleStrategy final : public strategy::IStrategy {
 public:
  ExampleStrategy();
  ~ExampleStrategy() override;

  ErrorCode Init(const strategy::StrategyConfig& config) override;
  ErrorCode Start() override;
  void Stop() override;

  void SetOrderSender(strategy::OrderSender sender) override;
  strategy::StrategyConfig GetStrategyConfig() const override;

  void OnTick(const qtrade_sdk::quote::MarketTick& tick) override;
  void OnBar(const qtrade_sdk::quote::Bar& bar) override;
  void OnOrder(const qtrade_sdk::trader::Order& order) override;
  void OnTrade(const qtrade_sdk::trader::Trade& trade) override;

 private:
  strategy::StrategyConfig config_;
  strategy::OrderSender order_sender_;
  bool running_ = false;
  double last_price_ = 0.0;
  int position_ = 0;  // -1 short, 0 flat, 1 long
};

/// @brief 工厂函数（非插件路径 / 测试用）
std::unique_ptr<strategy::IStrategy> CreateExampleStrategy();

}  // namespace qtrade::demo

#endif  // QTRADE_TRADE_DEMO_EXAMPLE_STRATEGY_HPP
