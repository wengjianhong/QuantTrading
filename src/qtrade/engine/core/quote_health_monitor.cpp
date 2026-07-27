/// @file      quote_health_monitor.cpp
/// @brief     行情健康监控实现
/// @author    wengjianhong
/// @date      2026-07-27
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/core/quote_health_monitor.hpp"

namespace qtrade::engine {

QuoteHealthMonitor::QuoteHealthMonitor() = default;

QuoteHealthMonitor::~QuoteHealthMonitor() {
  Stop();
}

void QuoteHealthMonitor::Start() {
  std::lock_guard lock(mutex_);
  if (running_) {
    return;
  }
  running_ = true;
  healthy_.store(false, std::memory_order_release);
  last_valid_tick_ms_.store(0, std::memory_order_release);
  health_worker_ = std::thread([this] { WatchHealth(); });
}

void QuoteHealthMonitor::Stop() {
  {
    std::lock_guard lock(mutex_);
    if (!running_) {
      return;
    }
    running_ = false;
    healthy_.store(false, std::memory_order_release);
  }
  health_cv_.notify_all();
  if (health_worker_.joinable()) {
    health_worker_.join();
  }
}

ErrorCode QuoteHealthMonitor::Configure(const QuoteHealthOptions& options) {
  if (options.max_stale_age <= std::chrono::milliseconds::zero()) {
    return ErrorCode::kSystemError;
  }
  std::lock_guard lock(mutex_);
  options_ = options;
  health_cv_.notify_all();
  return ErrorCode::kSuccess;
}

void QuoteHealthMonitor::SetHealthChangedHandler(HealthChangedHandler handler) {
  std::lock_guard lock(mutex_);
  health_changed_handler_ = std::move(handler);
}

void QuoteHealthMonitor::OnValidTick() {
  last_valid_tick_ms_.store(SteadyNowMs(), std::memory_order_release);
  const bool previous = healthy_.exchange(true, std::memory_order_acq_rel);
  if (!previous) {
    NotifyHealthChanged(true);
  }
}

void QuoteHealthMonitor::OnInvalidTick() {
  const bool previous = healthy_.exchange(false, std::memory_order_acq_rel);
  if (previous) {
    NotifyHealthChanged(false);
  }
}

bool QuoteHealthMonitor::IsHealthy() const {
  return healthy_.load(std::memory_order_acquire);
}

void QuoteHealthMonitor::NotifyHealthChanged(bool healthy) {
  HealthChangedHandler handler;
  {
    std::lock_guard lock(mutex_);
    handler = health_changed_handler_;
  }
  if (handler) {
    handler(healthy);
  }
}

void QuoteHealthMonitor::WatchHealth() {
  while (running_.load(std::memory_order_acquire)) {
    HealthChangedHandler handler;
    {
      std::unique_lock lock(mutex_);
      const auto interval = std::min(options_.max_stale_age, std::chrono::milliseconds(100));
      health_cv_.wait_for(lock, interval, [this] { return !running_.load(std::memory_order_acquire); });
      if (!running_.load(std::memory_order_acquire)) {
        return;
      }
      const auto last_tick = last_valid_tick_ms_.load(std::memory_order_acquire);
      if (!healthy_.load(std::memory_order_acquire) || last_tick == 0 ||
          SteadyNowMs() - last_tick <= options_.max_stale_age.count()) {
        continue;
      }
      healthy_.store(false, std::memory_order_release);
      handler = health_changed_handler_;
    }
    if (handler) {
      handler(false);
    }
  }
}

std::int64_t QuoteHealthMonitor::SteadyNowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
    .count();
}

}  // namespace qtrade::engine
