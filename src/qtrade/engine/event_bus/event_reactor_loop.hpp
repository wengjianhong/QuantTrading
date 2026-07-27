/// @file      event_reactor_loop.hpp
/// @brief     EventBus FIFO 有界队列 Reactor 循环（Demultiplex + RunOnce）
/// @details   单线程消费有界队列；满队列策略由 Policy（丢最旧或拒写）决定；
///            实现位于 .cpp，并对 EventPtr + Market/TraderLanePolicy 显式实例化
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EVENT_REACTOR_LOOP_HPP_
#define QTRADE_TRADING_ENGINE_EVENT_REACTOR_LOOP_HPP_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <utility>

namespace qtrade::engine::event_bus {

/// @brief 单 Reactor 线程的一次迭代结果
enum class RunOnceResult {
  /// 已出队一个事件，调用方应交给 EventHandler
  kHandled,
  /// 仍在运行但本次被虚假唤醒且队列为空
  kIdle,
  /// 已停止且队列已排空，Reactor 线程应退出
  kStopped,
};

/// @brief Lane-Q 队列策略：有界队列，满则丢弃最旧
struct QuoteLanePolicy {
  /// 队列容量上限
  static constexpr std::size_t kCapacity = 8192;
  /// 队列满时是否丢弃最旧事件
  static constexpr bool kDropOldestOnFull = true;
};

/// @brief Lane-T 队列策略：有界队列，满则拒写
struct TraderLanePolicy {
  /// 队列容量上限
  static constexpr std::size_t kCapacity = 8192;
  /// 队列满时是否丢弃最旧事件（false 表示拒写）
  static constexpr bool kDropOldestOnFull = false;
};

/// @brief FIFO 有界队列 + condition_variable 的 Reactor 循环
/// @tparam Event 队列元素类型
/// @tparam Policy 容量与满队列策略（需提供 kCapacity / kDropOldestOnFull）
template <typename Event, typename Policy>
class EventReactorLoop {
 public:
  /// @brief 构造 Reactor 循环
  /// @param name 日志与诊断用的车道名
  explicit EventReactorLoop(std::string_view name);

  /// @brief 禁止拷贝构造
  EventReactorLoop(const EventReactorLoop&) = delete;

  /// @brief 禁止拷贝赋值
  EventReactorLoop& operator=(const EventReactorLoop&) = delete;

  /// @brief 析构时 Stop，确保 Reactor 线程退出
  ~EventReactorLoop();

  /// @brief 启动 Reactor 线程
  /// @param handle_event 出队事件的处理回调
  void Start(std::function<void(const Event&)> handle_event);

  /// @brief 停止接收入队并 join Reactor 线程，清空队列
  void Stop();

  /// @brief 生产者入队；满队列策略由 Policy 决定
  /// @param event 待入队事件
  /// @return 入队成功返回 true；停写或拒写返回 false
  bool Publish(Event event);

  /// @brief Reactor 线程单次迭代：等待就绪 → 出队一个事件
  /// @return 迭代结果与可选事件（kHandled 时 event 有值）
  [[nodiscard]] std::pair<RunOnceResult, std::optional<Event>> RunOnce();

  /// @brief 查询队列是否仍有待处理事件
  /// @return 队列非空时返回 true
  [[nodiscard]] bool HasPending() const;

  /// @brief 获取当前队列深度
  /// @return 待处理事件个数
  [[nodiscard]] std::size_t PendingCount() const;

  /// @brief 获取因队列满而丢弃最旧事件的累计次数
  /// @return 丢弃计数
  [[nodiscard]] std::uint64_t DroppedCount() const;

  /// @brief 获取因停写或拒写而未入队的累计次数
  /// @return 拒写计数
  [[nodiscard]] std::uint64_t RejectedCount() const;

 private:
  /// @brief Reactor 线程主循环：反复 RunOnce 并回调业务 Handler
  /// @param handle_event 事件处理回调
  void RunLoop(const std::function<void(const Event&)>& handle_event);

  /// 日志与诊断用的车道名
  std::string_view name_;
  /// FIFO 事件队列
  std::deque<Event> queue_;
  /// 保护 queue_ 的互斥锁
  mutable std::mutex mutex_;
  /// 队列非空或停止时唤醒 Reactor
  std::condition_variable cv_;
  /// Reactor 专用线程
  std::thread reactor_thread_;
  /// Reactor 是否处于运行中
  std::atomic<bool> running_{false};
  /// 是否仍接受 Publish 入队
  std::atomic<bool> accepting_{false};
  /// 队列满时丢弃最旧事件的累计次数
  std::atomic<std::uint64_t> dropped_{0};
  /// 停写或拒写未入队的累计次数
  std::atomic<std::uint64_t> rejected_{0};
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_EVENT_REACTOR_LOOP_HPP_
