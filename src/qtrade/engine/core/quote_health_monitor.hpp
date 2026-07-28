/// @file      quote_health_monitor.hpp
/// @brief     行情健康监控
/// @details   跟踪有效 Tick 时间戳并在静默超时时通知引擎 READY 门禁
/// @author    wengjianhong
/// @date      2026-07-27
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_ENGINE_QUOTE_HEALTH_MONITOR_HPP_
#define QTRADE_ENGINE_QUOTE_HEALTH_MONITOR_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

namespace qtrade::engine {

/// @brief 行情健康检查参数
struct QuoteHealthOptions {
  /// 最后一笔有效行情允许的最大静默时间
  std::chrono::milliseconds max_stale_age = std::chrono::milliseconds(3000);
};

/// @brief 行情健康状态监控器
class QuoteHealthMonitor {
 public:
  /// @brief 健康状态变化回调
  using HealthChangedHandler = std::function<void(bool healthy)>;

  QuoteHealthMonitor();
  ~QuoteHealthMonitor();

  QuoteHealthMonitor(const QuoteHealthMonitor&) = delete;
  QuoteHealthMonitor& operator=(const QuoteHealthMonitor&) = delete;

  /// @brief 启动后台静默检测线程
  void Start();

  /// @brief 停止后台线程
  void Stop();

  /// @brief 配置健康检查参数
  /// @param options 健康检查参数
  /// @return 参数有效返回 kSuccess
  ErrorCode Configure(const QuoteHealthOptions& options);

  /// @brief 设置健康状态变化回调
  /// @param handler 健康状态翻转时调用
  void SetHealthChangedHandler(HealthChangedHandler handler);

  /// @brief 记录一笔有效 Tick
  void OnValidTick();

  /// @brief 记录一笔无效 Tick
  void OnInvalidTick();

  /// @brief 查询当前是否健康
  /// @return 已收到有效行情且未超时时返回 true
  [[nodiscard]] bool IsHealthy() const;

 private:
  void WatchHealth();
  void NotifyHealthChanged(bool healthy);
  [[nodiscard]] static std::int64_t SteadyNowMs();

  std::mutex mutex_;
  std::atomic_bool running_ = false;
  std::atomic_bool healthy_ = false;
  std::atomic<std::int64_t> last_valid_tick_ms_ = 0;
  HealthChangedHandler health_changed_handler_;
  QuoteHealthOptions options_;
  std::thread health_worker_;
  std::condition_variable health_cv_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_QUOTE_HEALTH_MONITOR_HPP_
