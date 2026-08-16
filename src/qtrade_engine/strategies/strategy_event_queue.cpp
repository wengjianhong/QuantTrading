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

namespace qtrade::engine::strategies {

StrategyEventQueue::StrategyEventQueue(qtrade::strategy::IStrategy& strategy) : strategy_(strategy) {}

StrategyEventQueue::~StrategyEventQueue() {
  Stop();
}

void StrategyEventQueue::Start() {
  if (running_.exchange(true)) {
    return;
  }
  accepting_.store(true, std::memory_order_release);
  worker_ = std::thread([this] { Run(); });
}

void StrategyEventQueue::Stop() {
  if (!running_.exchange(false)) {
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

bool StrategyEventQueue::Enqueue(StrategyEvent event) {
  {
    std::lock_guard lock(mutex_);
    if (!accepting_.load(std::memory_order_acquire)) {
      return false;
    }
    if (queue_.size() >= kQueueCapacity) {
      queue_.pop_front();
      spdlog::warn("[StrategyEventQueue] queue full, dropped oldest event");
    }
    queue_.push_back(std::move(event));
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
  }
}

}  // namespace qtrade::engine::strategies
