/// @file      trader_normalizer.cpp
/// @brief     交易标准化模块实现
/// @details   接收交易通道回报，校验过滤后发布至 Lane-T
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/normalizer/trader_normalizer.hpp"

#include <spdlog/spdlog.h>

#include <cmath>

namespace qtrade::engine::normalizer {

TraderNormalizer::TraderNormalizer(event_bus::TraderEventReactor& trader_event_reactor)
  : running_(false), trader_event_reactor_(trader_event_reactor) {
  (void)trader_event_reactor_;
}

TraderNormalizer::~TraderNormalizer() {
  Stop();
}

void TraderNormalizer::Start() {
  {
    std::lock_guard lock(mutex_);
    if (running_.load(std::memory_order_acquire)) {
      return;
    }
    running_.store(true, std::memory_order_release);
  }
  spdlog::info("[TraderNormalizer] started successfully");
}

void TraderNormalizer::Stop() {
  // 标记停止后断开交易通道
  qtrade_sdk::trader::TraderApi* trader_api = nullptr;
  {
    std::lock_guard lock(mutex_);
    if (!running_.load(std::memory_order_acquire)) {
      return;
    }
    running_.store(false, std::memory_order_release);
    trader_api = trader_api_.get();
  }
  if (trader_api != nullptr && trader_api->IsConnected()) {
    trader_api->Disconnect();
  }
  spdlog::info("[TraderNormalizer] stopped cleanly");
}

void TraderNormalizer::SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api) {
  std::lock_guard lock(mutex_);
  // 运行中不允许更换通道；设置后注册订单/成交回调
  if (running_.load(std::memory_order_acquire)) {
    return;
  }
  trader_api_ = std::move(trader_api);
  if (!trader_api_) {
    return;
  }
  trader_api_->SetOrderCallback([this](const qtrade_sdk::trader::Order& order) { OnOrder(order); });
  trader_api_->SetTradeCallback([this](const qtrade_sdk::trader::Trade& trade) { OnTrade(trade); });
}

qtrade_sdk::trader::TraderApi* TraderNormalizer::GetTraderApi() {
  std::lock_guard lock(mutex_);
  return trader_api_.get();
}

bool TraderNormalizer::IsHealthy() const {
  std::lock_guard lock(mutex_);
  return running_.load(std::memory_order_acquire) && trader_api_ != nullptr && trader_api_->IsConnected();
}

void TraderNormalizer::OnOrder(const qtrade_sdk::trader::Order& order) {
  if (!running_.load(std::memory_order_acquire) ||
      (order.order_id.empty() && order.order_emt_id == 0 && order.client_order_id == 0) || order.volume < 0 ||
      order.traded_volume < 0 || order.left_volume < 0 || (order.volume > 0 && order.traded_volume > order.volume)) {
    return;
  }
  trader_event_reactor_.PublishOrder(order);
}

void TraderNormalizer::OnTrade(const qtrade_sdk::trader::Trade& trade) {
  if (!running_.load(std::memory_order_acquire) || trade.instrument.empty() || trade.volume <= 0 ||
      !std::isfinite(trade.price) || trade.price < 0.0 ||
      (trade.order_id.empty() && trade.order_emt_id == 0 && trade.client_order_id == 0)) {
    return;
  }
  trader_event_reactor_.PublishTrade(trade);
}

}  // namespace qtrade::engine::normalizer
