/// @file      order_intent_queue.cpp
/// @brief     OrderIntent 队列与 E 段工作线程实现
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/order_intent_queue.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::engine {

OrderIntentQueue::OrderIntentQueue(account_risk::AccountRiskApi& account_risk,
                                   orders::OrderApi& orders,
                                   execution::ExecutionApi& execution)
  : account_risk_(account_risk), orders_(orders), execution_(execution) {}

OrderIntentQueue::~OrderIntentQueue() {
  Stop();
}

void OrderIntentQueue::Start() {
  if (running_.exchange(true)) {
    return;
  }
  accepting_.store(true, std::memory_order_release);
  worker_ = std::thread([this] { Run(); });
}

void OrderIntentQueue::Stop() {
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

ErrorCode OrderIntentQueue::Enqueue(OrderIntent intent) {
  {
    std::lock_guard lock(mutex_);
    if (!accepting_.load(std::memory_order_acquire)) {
      return ErrorCode::kNotInitialized;
    }
    if (queue_.size() >= kQueueCapacity) {
      return ErrorCode::kResourceExhausted;
    }
    queue_.push_back(std::move(intent));
  }
  cv_.notify_one();
  return ErrorCode::kSuccess;
}

void OrderIntentQueue::Run() {
  while (true) {
    OrderIntent intent;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return !running_.load(std::memory_order_acquire) || !queue_.empty(); });
      if (!running_.load(std::memory_order_acquire) && queue_.empty()) {
        return;
      }
      if (queue_.empty()) {
        continue;
      }
      intent = std::move(queue_.front());
      queue_.pop_front();
    }
    Execute(intent);
  }
}

void OrderIntentQueue::Execute(const OrderIntent& intent) {
  const auto& request = intent.request;
  const std::string order_id = orders_.AllocateOrderId();
  if (const auto rc = account_risk_.Reserve(request, order_id); rc != ErrorCode::kSuccess) {
    spdlog::warn("[OrderIntentQueue] Reserve failed order_id={} code={}", order_id, static_cast<int>(rc));
    return;
  }

  const auto order = orders_.CreateOrder(request, order_id);
  if (!order.has_value()) {
    account_risk_.Release(order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    spdlog::warn("[OrderIntentQueue] CreateOrder failed order_id={}", order_id);
    return;
  }

  const std::string& created_order_id = order->order_id;
  const auto lifecycle = orders_.GetLifecycleState(created_order_id);
  if (lifecycle.has_value() && *lifecycle != orders::OrderLifecycleState::kPrepared) {
    return;
  }

  if (const auto rc = orders_.MarkEmsQueued(created_order_id); rc != ErrorCode::kSuccess) {
    account_risk_.Release(created_order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    spdlog::warn("[OrderIntentQueue] MarkEmsQueued failed order_id={} code={}",
                 created_order_id,
                 static_cast<int>(rc));
    return;
  }

  const auto rc = execution_.Enqueue(*order);
  if (rc != ErrorCode::kSuccess) {
    (void)orders_.RecordSendResult(created_order_id, rc);
    account_risk_.Release(created_order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    spdlog::warn("[OrderIntentQueue] EMS Enqueue failed order_id={} code={}", created_order_id, static_cast<int>(rc));
  }
}

}  // namespace qtrade::engine
