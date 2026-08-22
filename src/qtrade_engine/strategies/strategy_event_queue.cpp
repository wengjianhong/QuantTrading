/// @file      strategy_event_queue.cpp
/// @brief     每策略串行事件队列实现
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/strategies/strategy_event_queue.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <type_traits>
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#endif

namespace qtrade::engine::strategies {
namespace {

void SetWorkerName(const char* name) {
#if defined(__linux__)
  pthread_setname_np(pthread_self(), name);
#else
  (void)name;
#endif
}

}  // namespace

StrategyEventQueue::StrategyEventQueue(qtrade::strategy::IStrategy& strategy, std::size_t capacity)
  : capacity_(capacity == 0 ? kDefaultCapacity : capacity), strategy_(strategy) {}

StrategyEventQueue::~StrategyEventQueue() {
  Stop();
}

void StrategyEventQueue::Start() {
  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    return;
  }
  try {
    worker_ = std::thread([this] {
      SetWorkerName("seq-worker");
      Run();
    });
  } catch (...) {
    running_.store(false, std::memory_order_release);
    throw;
  }
  accepting_.store(true, std::memory_order_release);
}

void StrategyEventQueue::Stop() {
  if (!running_.exchange(false, std::memory_order_acq_rel)) {
    return;
  }
  accepting_.store(false, std::memory_order_release);
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
  std::lock_guard lock(mutex_);
  queue_.clear();
}

bool StrategyEventQueue::EnqueueTick(const qtrade::sdk::quote::MarketTick& tick) {
  return Enqueue(tick);
}

bool StrategyEventQueue::EnqueueBar(const qtrade::sdk::quote::Bar& bar) {
  return Enqueue(bar);
}

bool StrategyEventQueue::EnqueueOrder(const qtrade::sdk::trader::Order& order) {
  return Enqueue(order);
}

bool StrategyEventQueue::EnqueueTrade(const qtrade::sdk::trader::Trade& trade) {
  return Enqueue(trade);
}

bool StrategyEventQueue::DropOldestMarketLocked(const char** dropped_kind) {
  for (auto it = queue_.begin(); it != queue_.end(); ++it) {
    if (std::holds_alternative<qtrade::sdk::quote::MarketTick>(*it)) {
      *dropped_kind = "Tick";
      queue_.erase(it);
      return true;
    }
    if (std::holds_alternative<qtrade::sdk::quote::Bar>(*it)) {
      *dropped_kind = "Bar";
      queue_.erase(it);
      return true;
    }
  }
  return false;
}

bool StrategyEventQueue::Enqueue(StrategyEvent event) {
  const char* dropped_kind = nullptr;
  {
    std::lock_guard lock(mutex_);
    if (!accepting_.load(std::memory_order_acquire)) {
      return false;
    }
    if (queue_.size() >= capacity_) {
      if (!DropOldestMarketLocked(&dropped_kind)) {
        return false;
      }
    }
    queue_.push_back(std::move(event));
  }
  if (dropped_kind != nullptr) {
    const auto n = dropped_count_.fetch_add(1, std::memory_order_relaxed) + 1;
    if (n == 1 || n % 256 == 0) {
      spdlog::debug("[StrategyEventQueue] queue full, dropped oldest {} (count={})", dropped_kind, n);
    }
  }
  cv_.notify_one();
  return true;
}

void StrategyEventQueue::Run() {
  while (true) {
    StrategyEvent event;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return !running_.load(std::memory_order_acquire) || !queue_.empty(); });
      if (!running_.load(std::memory_order_acquire) && queue_.empty()) {
        return;
      }
      if (queue_.empty()) {
        continue;
      }
      event = std::move(queue_.front());
      queue_.pop_front();
    }
    Dispatch(event);
  }
}

void StrategyEventQueue::Dispatch(const StrategyEvent& event) {
  try {
    std::visit(
      [this](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, qtrade::sdk::quote::MarketTick>) {
          strategy_.OnTick(payload);
        } else if constexpr (std::is_same_v<T, qtrade::sdk::quote::Bar>) {
          strategy_.OnBar(payload);
        } else if constexpr (std::is_same_v<T, qtrade::sdk::trader::Order>) {
          strategy_.OnOrder(payload);
        } else if constexpr (std::is_same_v<T, qtrade::sdk::trader::Trade>) {
          strategy_.OnTrade(payload);
        }
      },
      event);
  } catch (const std::exception& e) {
    spdlog::error("[StrategyEventQueue] strategy callback exception: {}", e.what());
  } catch (...) {
    spdlog::error("[StrategyEventQueue] strategy callback unknown exception");
  }
}

}  // namespace qtrade::engine::strategies
