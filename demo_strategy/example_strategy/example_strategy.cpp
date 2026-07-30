/// @file      example_strategy.cpp
/// @brief     示例策略实现
/// @details   实现简单趋势跟踪策略的 on_tick/on_bar 逻辑
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "example_strategy.hpp"

#include <spdlog/spdlog.h>

#include <cmath>

namespace qtrade::demo {

namespace quote = qtrade_sdk::quote;
namespace trader = qtrade_sdk::trader;

ExampleStrategy::ExampleStrategy() = default;

ExampleStrategy::~ExampleStrategy() {
  Stop();
}

ErrorCode ExampleStrategy::Init(const strategy::StrategyConfig& config) {
  config_ = config;
  spdlog::info("[ExampleStrategy] init strategy_id={} strategy_name={}", config_.strategy_id, config_.strategy_name);
  return ErrorCode::kSuccess;
}

ErrorCode ExampleStrategy::Start() {
  running_ = true;
  spdlog::info("[ExampleStrategy] started");
  return ErrorCode::kSuccess;
}

void ExampleStrategy::Stop() {
  running_ = false;
  spdlog::info("[ExampleStrategy] stopped");
}

void ExampleStrategy::SetOrderSender(strategy::OrderSender sender) {
  order_sender_ = std::move(sender);
}

strategy::StrategyConfig ExampleStrategy::GetStrategyConfig() const {
  return config_;
}

void ExampleStrategy::OnTick(const quote::MarketTick& tick) {
  if (!running_) {
    return;
  }

  double price_change = 0.0;
  if (last_price_ > 0) {
    price_change = (tick.last_price - last_price_) / last_price_;
  }
  last_price_ = tick.last_price;

  const double threshold = config_.order_threshold.value_or(0.005);
  const std::int64_t volume = config_.order_volume > 0 ? config_.order_volume : 1;
  if (std::fabs(price_change) <= threshold || !order_sender_) {
    return;
  }

  trader::OrderRequest request;
  request.instrument = tick.instrument;
  request.price = tick.last_price;
  request.volume = volume;

  if (price_change > 0 && position_ <= 0) {
    request.side = trader::SideType::kBuy;
    spdlog::info("[ExampleStrategy] buy signal on {} at {}", tick.instrument, tick.last_price);
    strategy::OrderBatch batch;
    batch.order_requests.push_back(request);
    order_sender_(batch);
    position_ = 1;
  } else if (price_change < 0 && position_ >= 0) {
    request.side = trader::SideType::kSell;
    spdlog::info("[ExampleStrategy] sell signal on {} at {}", tick.instrument, tick.last_price);
    strategy::OrderBatch batch;
    batch.order_requests.push_back(request);
    order_sender_(batch);
    position_ = -1;
  }
}

void ExampleStrategy::OnBar(const quote::Bar& bar) {
  if (!running_) {
    return;
  }
  spdlog::info("[ExampleStrategy] bar received: {} open={}, close={}", bar.instrument, bar.open, bar.close);
}

void ExampleStrategy::OnOrder(const trader::Order& order) {
  spdlog::info("[ExampleStrategy] order update: {} status={}", order.order_id, static_cast<int>(order.status));
}

void ExampleStrategy::OnTrade(const trader::Trade& trade) {
  spdlog::info("[ExampleStrategy] trade: {} price={}, volume={}", trade.instrument, trade.price, trade.volume);
}

std::unique_ptr<strategy::IStrategy> CreateExampleStrategy() {
  return std::make_unique<ExampleStrategy>();
}

}  // namespace qtrade::demo
