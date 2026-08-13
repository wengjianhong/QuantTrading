/// @file      quote_health_monitor.cpp
/// @brief     行情健康监控实现
/// @details   有效/无效 Tick 更新健康态；WatchHealth 线程检测 quote_max_stale_ms 静默超时。
/// @author    wengjianhong
/// @date      2026-07-27
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/core/quote_health_monitor.hpp"

#include "qtrade/common/system/time.hpp"

#include <chrono>

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
  // 1. 置停止标志并唤醒后台线程
  {
    std::lock_guard lock(mutex_);
    if (!running_) {
      return;
    }
    running_ = false;
    healthy_.store(false, std::memory_order_release);
  }
  health_cv_.notify_all();

  // 2. 等待工作线程退出
  if (health_worker_.joinable()) {
    health_worker_.join();
  }
}

ErrorCode QuoteHealthMonitor::Configure(const QuoteHealthOptions& options) {
  if (options.quote_max_stale_ms <= 0) {
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
  last_valid_tick_ms_.store(qtrade::common::system::SteadyMillisNow(), std::memory_order_release);
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
      // 1. 周期性等待；Stop/Configure 时提前唤醒
      const auto stale_ms = std::chrono::milliseconds(options_.quote_max_stale_ms);
      const auto interval = std::min(stale_ms, std::chrono::milliseconds(100));
      health_cv_.wait_for(lock, interval, [this] { return !running_.load(std::memory_order_acquire); });
      if (!running_.load(std::memory_order_acquire)) {
        return;
      }

      // 2. 检测静默超时
      const auto last_tick = last_valid_tick_ms_.load(std::memory_order_acquire);
      if (!healthy_.load(std::memory_order_acquire) || last_tick == 0 ||
          qtrade::common::system::SteadyMillisNow() - last_tick <= options_.quote_max_stale_ms) {
        continue;
      }
      healthy_.store(false, std::memory_order_release);
      handler = health_changed_handler_;
    }

    // 3. 锁外通知引擎（不健康）
    if (handler) {
      handler(false);
    }
  }
}

}  // namespace qtrade::engine
