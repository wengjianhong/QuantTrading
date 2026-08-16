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
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

namespace qtrade::engine {

/// @brief 行情健康检查参数
struct QuoteHealthOptions {
  /// 最后一笔有效 Tick 允许的最大静默时间（毫秒）；须 > 0
  std::int32_t quote_max_stale_ms = 3000;
};

/// @brief 行情健康状态监控器
/// @details 由 Lane-Q 有效 Tick 驱动健康翻转；后台线程检测静默超时并回调引擎生命周期。
class QuoteHealthMonitor {
 public:
  /// @brief 健康状态变化回调
  using HealthChangedHandler = std::function<void(bool healthy)>;

  /// @brief 构造监控器
  QuoteHealthMonitor();

  /// @brief 析构并停止后台线程
  ~QuoteHealthMonitor();

  /// @brief 禁止移动构造
  QuoteHealthMonitor(QuoteHealthMonitor&& other) = delete;
  /// @brief 禁止拷贝构造
  QuoteHealthMonitor(const QuoteHealthMonitor&) = delete;
  /// @brief 禁止移动赋值
  QuoteHealthMonitor& operator=(QuoteHealthMonitor&& other) = delete;
  /// @brief 禁止拷贝赋值
  QuoteHealthMonitor& operator=(const QuoteHealthMonitor&) = delete;

  /// @brief 启动后台静默检测线程
  void Start();

  /// @brief 停止后台线程并 join
  void Stop();

  /// @brief 配置健康检查参数
  /// @param options 健康检查参数
  /// @return 参数有效返回 kSuccess；quote_max_stale_ms 非正返回 kSystemError
  ErrorCode Configure(const QuoteHealthOptions& options);

  /// @brief 设置健康状态变化回调
  /// @param handler 健康状态翻转时在监控线程外同步调用；可为空
  void SetHealthChangedHandler(HealthChangedHandler handler);

  /// @brief 记录一笔有效 Tick 并刷新最后有效时间
  void OnValidTick();

  /// @brief 记录一笔无效 Tick 并立即标记不健康
  void OnInvalidTick();

  /// @brief 查询当前是否健康
  /// @return 已收到有效行情且未超时时返回 true
  [[nodiscard]] bool IsHealthy() const;

 private:
  /// @brief 后台循环：等待间隔后检测静默超时
  /// @details 超时则将 healthy_ 置 false 并触发回调；Stop 时通过 condition_variable 唤醒退出
  void WatchHealth();

  /// @brief 在锁外调用已注册的健康变化回调
  /// @param healthy 当前健康状态
  void NotifyHealthChanged(bool healthy);

  /// 保护 options_、handler 与 Start/Stop 状态
  std::mutex mutex_;
  /// 后台检测线程是否运行中
  std::atomic<bool> running_ = false;
  /// 当前行情是否健康（有效 Tick 且未静默超时）
  std::atomic<bool> healthy_ = false;
  /// 最后一笔有效 Tick 的单调时钟毫秒（SteadyMillisNow）
  std::atomic<std::int64_t> last_valid_tick_ms_ = 0;
  /// 健康翻转回调
  HealthChangedHandler health_changed_handler_;
  /// 健康检查参数快照
  QuoteHealthOptions options_;
  /// 静默检测工作线程
  std::thread health_worker_;
  /// 用于 Stop/Configure 时唤醒 WatchHealth
  std::condition_variable health_cv_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_QUOTE_HEALTH_MONITOR_HPP_
