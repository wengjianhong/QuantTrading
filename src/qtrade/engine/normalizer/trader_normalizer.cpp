/// @file      trader_normalizer.cpp
/// @brief     交易标准化模块实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/normalizer/trader_normalizer.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::normalizer {

TraderNormalizer::TraderNormalizer(event_bus::ReturnEventReactor& return_event_reactor)
  : return_event_reactor_(return_event_reactor), running_(false) {
  (void)return_event_reactor_;
}

TraderNormalizer::~TraderNormalizer() {
  Stop();
}

void TraderNormalizer::Start() {
  if (running_) {
    return;
  }
  running_ = true;
  spdlog::info("[TraderNormalizer] started successfully");
}

void TraderNormalizer::Stop() {
  if (!running_) {
    return;
  }
  if (trader_api_ && trader_api_->IsConnected()) {
    trader_api_->Disconnect();
  }
  running_ = false;
  spdlog::info("[TraderNormalizer] stopped cleanly");
}

void TraderNormalizer::SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api) {
  trader_api_ = std::move(trader_api);
  if (!trader_api_) {
    return;
  }
  trader_api_->SetOrderCallback([this](const qtrade_sdk::trader::Order& order) { OnOrder(order); });
  trader_api_->SetTradeCallback([this](const qtrade_sdk::trader::Trade& trade) { OnTrade(trade); });
}

qtrade_sdk::trader::TraderApi* TraderNormalizer::GetTraderApi() {
  return trader_api_.get();
}

void TraderNormalizer::OnOrder(const qtrade_sdk::trader::Order& order) {
  if (running_) {
    return_event_reactor_.PublishOrder(order);
  }
}

void TraderNormalizer::OnTrade(const qtrade_sdk::trader::Trade& trade) {
  if (running_) {
    return_event_reactor_.PublishTrade(trade);
  }
}

}  // namespace qtrade::engine::normalizer
