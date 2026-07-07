/// @file      quote_normalizer.cpp
/// @brief     行情标准化模块实现
/// @details   接收协议适配器回调，语义标准化后发布至 Lane-M
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#include "engine/normalizer/quote_normalizer.hpp"

#include <qtrade/error_code/code_message.hpp>

#include <spdlog/spdlog.h>

namespace qtrade::engine::normalizer {

QuoteNormalizer::QuoteNormalizer(event_bus::MarketEventReactor& market_event_reactor)
  : market_event_reactor_(market_event_reactor), running_(false) {}

QuoteNormalizer::~QuoteNormalizer() { Stop(); }

void QuoteNormalizer::Start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) {
    return;
  }
  running_ = true;
  spdlog::info("[QuoteNormalizer] started successfully");
}

void QuoteNormalizer::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return;
  }
  if (market_source_ && market_source_->IsConnected()) {
    market_source_->Disconnect();
  }
  running_ = false;
  spdlog::info("[QuoteNormalizer] stopped cleanly");
}

void QuoteNormalizer::SetQuoteApi(std::unique_ptr<qtrade_sdk::quote::QuoteApi> source) {
  std::lock_guard<std::mutex> lock(mutex_);
  market_source_ = std::move(source);

  if (market_source_) {
    market_source_->SetTickCallback([this](const qtrade_sdk::quote::MarketTick& tick) { OnTick(tick); });
    market_source_->SetBarCallback([this](const qtrade_sdk::quote::Bar& bar) { OnBar(bar); });
    spdlog::info("[QuoteNormalizer] quote api set successfully");
  }
}

qtrade_sdk::quote::QuoteApi* QuoteNormalizer::GetQuoteApi() {
  std::lock_guard<std::mutex> lock(mutex_);
  return market_source_.get();
}

void QuoteNormalizer::Subscribe(const std::vector<std::string>& instruments) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!market_source_ || !running_) {
    spdlog::warn("[QuoteNormalizer] cannot subscribe: quote api not ready");
    return;
  }
  auto rc = market_source_->Subscribe({instruments});
  if (rc == ErrorCode::kSuccess) {
    spdlog::info("[QuoteNormalizer] subscribed to {} instruments", instruments.size());
  } else {
    spdlog::error("[QuoteNormalizer] subscription failed: {}", GetErrorCodeMessage(rc));
  }
}

void QuoteNormalizer::Unsubscribe(const std::vector<std::string>& instruments) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!market_source_ || !running_) {
    return;
  }
  market_source_->Unsubscribe({instruments});
  spdlog::info("[QuoteNormalizer] unsubscribed from {} instruments", instruments.size());
}

void QuoteNormalizer::OnTick(const qtrade_sdk::quote::MarketTick& tick) {
  // TODO: 跨柜台/交易所语义标准化（精度、合约代码、状态枚举等）
  market_event_reactor_.PublishTick(tick);
}

void QuoteNormalizer::OnBar(const qtrade_sdk::quote::Bar& bar) {
  // TODO: 跨柜台/交易所语义标准化
  market_event_reactor_.PublishBar(bar);
}

}  // namespace qtrade::engine::normalizer
