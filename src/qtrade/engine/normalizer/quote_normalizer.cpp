/// @file      quote_normalizer.cpp
/// @brief     行情标准化模块实现
/// @details   接收协议适配器回调，语义标准化后发布至 Lane-M
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/normalizer/quote_normalizer.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <cmath>

namespace qtrade::engine::normalizer {
namespace {

/// @brief 当前 steady_clock 毫秒
/// @return 自 epoch 起的毫秒数
std::int64_t SteadyNowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
    .count();
}

}  // namespace

QuoteNormalizer::QuoteNormalizer(event_bus::QuoteEventReactor& quote_event_reactor)
  : quote_event_reactor_(quote_event_reactor), running_(false) {}

QuoteNormalizer::~QuoteNormalizer() {
  Stop();
}

void QuoteNormalizer::Start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) {
    return;
  }
  running_ = true;
  healthy_.store(false, std::memory_order_release);
  last_valid_tick_ms_.store(0, std::memory_order_release);
  health_worker_ = std::thread([this] { WatchHealth(); });
  spdlog::info("[QuoteNormalizer] started successfully");
}

void QuoteNormalizer::Stop() {
  // 1. 标记停止并取出行情源指针
  qtrade_sdk::quote::QuoteApi* source = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
      return;
    }
    running_ = false;
    healthy_.store(false, std::memory_order_release);
    source = market_source_.get();
  }
  // 2. 唤醒健康线程、断开行情源并 join
  health_cv_.notify_all();
  if (source != nullptr && source->IsConnected()) {
    source->Disconnect();
  }
  if (health_worker_.joinable()) {
    health_worker_.join();
  }
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

void QuoteNormalizer::SetHealthHandler(HealthHandler handler) {
  std::lock_guard lock(mutex_);
  health_handler_ = std::move(handler);
}

ErrorCode QuoteNormalizer::ConfigureHealth(const QuoteHealthOptions& options) {
  if (options.max_stale_age <= std::chrono::milliseconds::zero()) {
    return ErrorCode::kSystemError;
  }
  std::lock_guard lock(mutex_);
  health_options_ = options;
  health_cv_.notify_all();
  return ErrorCode::kSuccess;
}

bool QuoteNormalizer::IsHealthy() const {
  return healthy_.load(std::memory_order_acquire);
}

void QuoteNormalizer::OnTick(const qtrade_sdk::quote::MarketTick& tick) {
  if (!running_.load(std::memory_order_acquire)) {
    return;
  }

  // 1. 校验 Tick 并更新健康时间戳
  const bool valid = !tick.instrument.empty() && tick.data_time > 0 && std::isfinite(tick.last_price) &&
                     tick.last_price > 0.0 && tick.volume >= 0;
  if (valid) {
    last_valid_tick_ms_.store(SteadyNowMs(), std::memory_order_release);
  }
  // 2. 健康状态翻转时回调；有效 Tick 再投递 Lane-M
  const bool previous = healthy_.exchange(valid, std::memory_order_acq_rel);
  if (previous != valid) {
    HealthHandler handler;
    {
      std::lock_guard lock(mutex_);
      handler = health_handler_;
    }
    if (handler) {
      handler(valid);
    }
  }
  if (!valid) {
    spdlog::warn("[QuoteNormalizer] rejected invalid tick: instrument={}", tick.instrument);
    return;
  }
  quote_event_reactor_.PublishTick(tick);
}

void QuoteNormalizer::OnBar(const qtrade_sdk::quote::Bar& bar) {
  if (!running_.load(std::memory_order_acquire) || bar.instrument.empty() || bar.open_time <= 0 ||
      bar.close_time < bar.open_time || !std::isfinite(bar.open) || !std::isfinite(bar.high) ||
      !std::isfinite(bar.low) || !std::isfinite(bar.close) || bar.high < bar.low || bar.volume < 0) {
    return;
  }
  quote_event_reactor_.PublishBar(bar);
}

void QuoteNormalizer::WatchHealth() {
  while (running_.load(std::memory_order_acquire)) {
    HealthHandler handler;
    {
      // 1. 周期性检查；超时则标记不健康
      std::unique_lock lock(mutex_);
      const auto interval = std::min(health_options_.max_stale_age, std::chrono::milliseconds(100));
      health_cv_.wait_for(lock, interval, [this] { return !running_.load(std::memory_order_acquire); });
      if (!running_.load(std::memory_order_acquire)) {
        return;
      }
      const auto last_tick = last_valid_tick_ms_.load(std::memory_order_acquire);
      if (!healthy_.load(std::memory_order_acquire) || last_tick == 0 ||
          SteadyNowMs() - last_tick <= health_options_.max_stale_age.count()) {
        continue;
      }
      healthy_.store(false, std::memory_order_release);
      handler = health_handler_;
    }
    // 2. 锁外通知健康丢失
    if (handler) {
      handler(false);
    }
  }
}

}  // namespace qtrade::engine::normalizer
