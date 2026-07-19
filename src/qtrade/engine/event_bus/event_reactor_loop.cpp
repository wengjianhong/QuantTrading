/// @file      event_reactor_loop.cpp
/// @brief     EventReactorLoop 模板实现与显式实例化
/// @details   仅为 EventPtr + MarketLanePolicy / ReturnLanePolicy 提供实例化
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/event_bus/event_reactor_loop.hpp"

#include "qtrade/engine/event_bus/event_types.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

template <typename Event, typename Policy>
EventReactorLoop<Event, Policy>::EventReactorLoop(std::string_view name) : name_(name) {}

template <typename Event, typename Policy>
EventReactorLoop<Event, Policy>::~EventReactorLoop() {
  Stop();
}

template <typename Event, typename Policy>
void EventReactorLoop<Event, Policy>::Start(std::function<void(const Event&)> handle_event) {
  if (running_.exchange(true)) {
    return;
  }
  accepting_.store(true, std::memory_order_release);
  reactor_thread_ = std::thread([this, handler = std::move(handle_event)] { RunLoop(handler); });
  spdlog::info("[{}] reactor thread started", name_);
}

template <typename Event, typename Policy>
void EventReactorLoop<Event, Policy>::Stop() {
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

template <typename Event, typename Policy>
bool EventReactorLoop<Event, Policy>::Publish(Event event) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!accepting_.load(std::memory_order_acquire)) {
      ++rejected_;
      return false;
    }
    if (queue_.size() >= Policy::kCapacity) {
      if constexpr (Policy::kDropOldestOnFull) {
        queue_.pop_front();
        ++dropped_;
        spdlog::warn("[{}] queue full, dropped oldest event", name_);
      } else {
        ++rejected_;
        spdlog::error("[{}] queue full, rejected publish", name_);
        return false;
      }
    }
    queue_.push_back(std::move(event));
  }
  cv_.notify_one();
  return true;
}

template <typename Event, typename Policy>
std::pair<RunOnceResult, std::optional<Event>> EventReactorLoop<Event, Policy>::RunOnce() {
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

template <typename Event, typename Policy>
bool EventReactorLoop<Event, Policy>::HasPending() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return !queue_.empty();
}

template <typename Event, typename Policy>
std::size_t EventReactorLoop<Event, Policy>::PendingCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

template <typename Event, typename Policy>
std::uint64_t EventReactorLoop<Event, Policy>::DroppedCount() const {
  return dropped_.load(std::memory_order_relaxed);
}

template <typename Event, typename Policy>
std::uint64_t EventReactorLoop<Event, Policy>::RejectedCount() const {
  return rejected_.load(std::memory_order_relaxed);
}

template <typename Event, typename Policy>
void EventReactorLoop<Event, Policy>::RunLoop(const std::function<void(const Event&)>& handle_event) {
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

template class EventReactorLoop<EventPtr, MarketLanePolicy>;
template class EventReactorLoop<EventPtr, ReturnLanePolicy>;

}  // namespace qtrade::engine::event_bus
