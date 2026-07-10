/// @file      event_reactor_loop.hpp
/// @brief     EventBus FIFO 有界队列 Reactor 循环（Demultiplex + RunOnce）
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

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

/// @brief 单 Reactor 线程的一次迭代结果
enum class RunOnceResult {
  kHandled,  ///< 已出队一个事件，调用方应交给 EventHandler
  kIdle,     ///< 仍在运行但本次被虚假唤醒且队列为空
  kStopped,  ///< 已停止且队列已排空，Reactor 线程应退出
};

/// @brief Lane-M 队列策略：有界队列，满则丢弃最旧
struct MarketLanePolicy {
  static constexpr std::size_t kCapacity = 8192;
  static constexpr bool kDropOldestOnFull = true;
};

/// @brief Lane-R 队列策略：有界队列，满则拒写
struct ReturnLanePolicy {
  static constexpr std::size_t kCapacity = 8192;
  static constexpr bool kDropOldestOnFull = false;
};

/// @brief FIFO 有界队列 + condition_variable 的 Reactor 循环
template <typename Event, typename Policy>
class EventReactorLoop {
 public:
  explicit EventReactorLoop(std::string_view name) : name_(name) {}

  EventReactorLoop(const EventReactorLoop&) = delete;
  EventReactorLoop& operator=(const EventReactorLoop&) = delete;

  ~EventReactorLoop() {
    Stop();
  }

  void Start(std::function<void(const Event&)> handle_event) {
    if (running_.exchange(true)) {
      return;
    }
    accepting_.store(true, std::memory_order_release);
    reactor_thread_ = std::thread([this, handler = std::move(handle_event)] { RunLoop(handler); });
    spdlog::info("[{}] reactor thread started", name_);
  }

  void Stop() {
    if (!running_.exchange(false)) {
      return;
    }
    accepting_.store(false, std::memory_order_release);
    cv_.notify_all();
    if (reactor_thread_.joinable()) {
      reactor_thread_.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    spdlog::info("[{}] reactor stopped cleanly", name_);
  }

  /// @brief 生产者入队；满队列策略由 Policy 决定
  bool Publish(Event event) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!accepting_.load(std::memory_order_acquire)) {
        return false;
      }
      if (queue_.size() >= Policy::kCapacity) {
        if constexpr (Policy::kDropOldestOnFull) {
          queue_.pop_front();
          spdlog::warn("[{}] queue full, dropped oldest event", name_);
        } else {
          spdlog::error("[{}] queue full, rejected publish", name_);
          return false;
        }
      }
      queue_.push_back(std::move(event));
    }
    cv_.notify_one();
    return true;
  }

  /// @brief Reactor 线程单次迭代：等待就绪 → 出队一个事件
  [[nodiscard]] std::pair<RunOnceResult, std::optional<Event>> RunOnce() {
    Event event;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !running_.load() || !queue_.empty(); });
      if (!running_.load() && queue_.empty()) {
        return {RunOnceResult::kStopped, std::nullopt};
      }
      if (queue_.empty()) {
        return {RunOnceResult::kIdle, std::nullopt};
      }
      event = std::move(queue_.front());
      queue_.pop_front();
    }
    return {RunOnceResult::kHandled, std::move(event)};
  }

  [[nodiscard]] bool HasPending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !queue_.empty();
  }

  [[nodiscard]] std::size_t PendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

 private:
  void RunLoop(const std::function<void(const Event&)>& handle_event) {
    while (true) {
      auto [result, event] = RunOnce();
      if (result == RunOnceResult::kStopped) {
        break;
      }
      if (result == RunOnceResult::kIdle) {
        continue;
      }
      handle_event(*event);
    }
  }

  std::string_view name_;
  std::deque<Event> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::thread reactor_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> accepting_{false};
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_EVENT_REACTOR_LOOP_HPP_
