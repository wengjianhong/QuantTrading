/// @file      event_reactor_loop.cpp
/// @brief     EventReactorLoop 模板实现与显式实例化
/// @details   仅为 EventPtr + QuoteLanePolicy / TraderLanePolicy 提供实例化
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/event_bus/event_reactor_loop.hpp"

#include "qtrade/engine/event_bus/event_types.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

template <typename Event>
EventReactorLoop<Event>::EventReactorLoop(std::string_view name) : name_(name) {}

template <typename Event>
void EventReactorLoop<Event>::SetLanePolicy(LanePolicy policy) {
  std::lock_guard<std::mutex> lock(mutex_);

  // 如果 Reactor 正在运行，则忽略设置队列策略
  if (running_.load(std::memory_order_acquire)) {
    spdlog::warn("[{}] ignore SetLanePolicy while reactor is running", name_);
    return;
  }

  // 设置队列策略
  policy_ = policy;
}

template <typename Event>
EventReactorLoop<Event>::~EventReactorLoop() {
  Stop();
}

template <typename Event>
void EventReactorLoop<Event>::Start(std::function<void(const Event&)> handle_event) {
  // 1. 原子防止重复启动，并恢复生产者入队能力
  if (running_.exchange(true)) {
    return;
  }
  accepting_.store(true, std::memory_order_release);
  reactor_thread_ = std::thread([this, handler = std::move(handle_event)] { RunLoop(handler); });
  spdlog::info("[{}] reactor thread started", name_);
}

template <typename Event>
void EventReactorLoop<Event>::Stop() {
  // 1. 先关闭生产者入口，再唤醒并回收消费线程
  if (!running_.exchange(false)) {
    return;
  }
  accepting_.store(false, std::memory_order_release);
  cv_.notify_all();
  if (reactor_thread_.joinable()) {
    reactor_thread_.join();
  }
  // 2. 线程退出后清理尚未消费的事件
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.clear();
  spdlog::info("[{}] reactor stopped cleanly", name_);
}

template <typename Event>
bool EventReactorLoop<Event>::Publish(Event event) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    // 1. 检查停写状态与满队列策略
    if (!accepting_.load(std::memory_order_acquire)) {
      ++rejected_;
      return false;
    }
    if (queue_.size() >= policy_.capacity) {
      if (policy_.drop_oldest_on_full) {
        queue_.pop_front();
        ++dropped_;
        spdlog::warn("[{}] queue full, dropped oldest event", name_);
      } else {
        ++rejected_;
        spdlog::error("[{}] queue full, rejected publish", name_);
        return false;
      }
    }
    // 2. 事件入队后通知一个消费者
    queue_.push_back(std::move(event));
  }
  cv_.notify_one();
  return true;
}

template <typename Event>
std::pair<RunOnceResult, std::optional<Event>> EventReactorLoop<Event>::RunOnce() {
  Event event;
  {
    // 等待事件入队或停止
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !running_.load() || !queue_.empty(); });

    // 检查停止状态且队列为空
    if (!running_.load() && queue_.empty()) {
      return {RunOnceResult::kStopped, std::nullopt};
    }

    // 检查队列为空
    if (queue_.empty()) {
      return {RunOnceResult::kIdle, std::nullopt};
    }

    // 出队事件
    event = std::move(queue_.front());
    queue_.pop_front();
  }
  return {RunOnceResult::kHandled, std::move(event)};
}

template <typename Event>
bool EventReactorLoop<Event>::HasPending() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return !queue_.empty();
}

template <typename Event>
std::size_t EventReactorLoop<Event>::PendingCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

template <typename Event>
std::size_t EventReactorLoop<Event>::DroppedCount() const {
  return dropped_.load(std::memory_order_relaxed);
}

template <typename Event>
std::size_t EventReactorLoop<Event>::RejectedCount() const {
  return rejected_.load(std::memory_order_relaxed);
}

template <typename Event>
void EventReactorLoop<Event>::RunLoop(const std::function<void(const Event&)>& handle_event) {
  // 消费线程主循环：阻塞出队 → 分发 handler，直到 Stop 且队列排空
  while (true) {
    auto [result, event] = RunOnce();

    // 停止且数据已经排空
    if (result == RunOnceResult::kStopped) {
      break;
    }

    // 队列为空，继续等待
    if (result == RunOnceResult::kIdle) {
      continue;
    }

    // 处理事件
    handle_event(*event);
  }
}

template class EventReactorLoop<EventPtr>;

}  // namespace qtrade::engine::event_bus
