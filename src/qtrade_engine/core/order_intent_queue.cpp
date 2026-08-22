/// @file      order_intent_queue.cpp
/// @brief     OrderIntent 队列与 E 段工作线程实现
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/order_intent_queue.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#endif

namespace qtrade::engine {
namespace {

qtrade::sdk::trader::Order MakeOrderFromRequest(const qtrade::sdk::trader::OrderRequest& request,
                                                const std::string& order_id) {
  qtrade::sdk::trader::Order order;
  order.order_id = order_id;
  order.client_order_id = request.client_order_id;
  order.broker_order_id = request.broker_order_id;
  order.instrument = request.instrument;
  order.market = request.market;
  order.price = request.price;
  order.volume = request.volume;
  order.left_volume = request.volume;
  order.price_type = request.price_type;
  order.side = request.side;
  order.position_effect = request.position_effect;
  order.business_type = request.business_type;
  return order;
}

void SetWorkerName(const char* name) {
#if defined(__linux__)
  pthread_setname_np(pthread_self(), name);
#else
  (void)name;
#endif
}

}  // namespace

OrderIntentQueue::OrderIntentQueue(account_risk::AccountRiskApi& account_risk,
                                   orders::OrderApi& orders,
                                   execution::ExecutionApi& execution,
                                   std::size_t capacity)
  : capacity_(capacity == 0 ? kDefaultCapacity : capacity),
    account_risk_(account_risk),
    orders_(orders),
    execution_(execution) {}

OrderIntentQueue::~OrderIntentQueue() {
  Stop();
}

void OrderIntentQueue::SetOutcomeHandler(OrderOutcomeHandler handler) {
  outcome_handler_ = std::move(handler);
}

void OrderIntentQueue::Start() {
  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    return;
  }
  try {
    worker_ = std::thread([this] {
      SetWorkerName("oiq-worker");
      Run();
    });
  } catch (...) {
    running_.store(false, std::memory_order_release);
    throw;
  }
  accepting_.store(true, std::memory_order_release);
}

void OrderIntentQueue::Stop() {
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
  inflight_client_ids_.clear();
  pending_count_.store(0, std::memory_order_release);
  pending_notional_.store(0.0, std::memory_order_release);
}

ErrorCode OrderIntentQueue::Enqueue(OrderIntent intent) {
  const auto client_id = intent.request.client_order_id;
  const double notional = intent.request.price * static_cast<double>(intent.request.volume);
  {
    std::lock_guard lock(mutex_);
    if (!accepting_.load(std::memory_order_acquire)) {
      return ErrorCode::kNotInitialized;
    }
    if (client_id != 0) {
      if (inflight_client_ids_.contains(client_id) || orders_.GetOrderByClientId(client_id).has_value()) {
        return ErrorCode::kSuccess;
      }
    }
    if (queue_.size() >= capacity_) {
      return ErrorCode::kResourceExhausted;
    }
    if (client_id != 0) {
      inflight_client_ids_.insert(client_id);
    }
    queue_.push_back(std::move(intent));
    pending_count_.fetch_add(1, std::memory_order_relaxed);
    pending_notional_.fetch_add(notional, std::memory_order_relaxed);
  }
  cv_.notify_one();
  return ErrorCode::kSuccess;
}

std::uint64_t OrderIntentQueue::PendingCount() const {
  return pending_count_.load(std::memory_order_acquire);
}

double OrderIntentQueue::PendingNotional() const {
  return pending_notional_.load(std::memory_order_acquire);
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
    try {
      Execute(intent);
    } catch (const std::exception& e) {
      spdlog::error("[OrderIntentQueue] Execute exception: {}", e.what());
    } catch (...) {
      spdlog::error("[OrderIntentQueue] Execute unknown exception");
    }
  }
}

void OrderIntentQueue::CompleteIntent(std::uint32_t client_order_id, double notional) {
  std::lock_guard lock(mutex_);
  if (client_order_id != 0) {
    inflight_client_ids_.erase(client_order_id);
  }
  pending_count_.fetch_sub(1, std::memory_order_relaxed);
  pending_notional_.fetch_sub(notional, std::memory_order_relaxed);
}

void OrderIntentQueue::NotifyOutcome(const qtrade::sdk::trader::Order& order) const {
  if (outcome_handler_) {
    outcome_handler_(order);
  }
}

void OrderIntentQueue::RecordLocalFailure(const qtrade::sdk::trader::OrderRequest& request,
                                          const std::string& order_id,
                                          ErrorCode rc) {
  auto existing = orders_.GetOrder(order_id);
  if (!existing.has_value()) {
    existing = orders_.CreateOrder(request, order_id);
  }
  if (existing.has_value() && existing->order_id != order_id) {
    return;
  }
  qtrade::sdk::trader::Order report = existing.value_or(MakeOrderFromRequest(request, order_id));
  if (rc == ErrorCode::kTimeout) {
    report.status = qtrade::sdk::trader::OrderStatusType::kUnknown;
  } else {
    report.status = qtrade::sdk::trader::OrderStatusType::kRejected;
    report.submit_status = qtrade::sdk::trader::OrderSubmitStatusType::kInsertRejected;
  }
  if (existing.has_value()) {
    report.order_id = existing->order_id;
    orders_.ApplyOrderReport(report);
    if (const auto latest = orders_.GetOrder(existing->order_id); latest.has_value()) {
      report = *latest;
    }
  }
  NotifyOutcome(report);
}

void OrderIntentQueue::Execute(const OrderIntent& intent) {
  const auto& request = intent.request;
  const auto client_id = request.client_order_id;
  const double notional = request.price * static_cast<double>(request.volume);
  struct Finisher {
    OrderIntentQueue* self;
    std::uint32_t client_id;
    double notional;
    ~Finisher() {
      self->CompleteIntent(client_id, notional);
    }
  } finisher{this, client_id, notional};

  try {
    const std::string order_id = orders_.AllocateOrderId();
    const auto reserve_rc = account_risk_.Reserve(request, order_id);
    const bool reserved = reserve_rc == ErrorCode::kSuccess;
    if (!reserved) {
      spdlog::warn("[OrderIntentQueue] Reserve failed order_id={} code={}", order_id, static_cast<int>(reserve_rc));
      RecordLocalFailure(request, order_id, reserve_rc);
      return;
    }

    const auto order = orders_.CreateOrder(request, order_id);
    if (!order.has_value()) {
      account_risk_.Release(order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
      auto report = MakeOrderFromRequest(request, order_id);
      report.status = qtrade::sdk::trader::OrderStatusType::kRejected;
      report.submit_status = qtrade::sdk::trader::OrderSubmitStatusType::kInsertRejected;
      NotifyOutcome(report);
      spdlog::warn("[OrderIntentQueue] CreateOrder failed order_id={}", order_id);
      return;
    }
    if (order->order_id != order_id) {
      account_risk_.Release(order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
      return;
    }

    const auto lifecycle = orders_.GetLifecycleState(order->order_id);
    if (lifecycle.has_value() && *lifecycle != orders::OrderLifecycleState::kPrepared) {
      return;
    }

    if (const auto rc = orders_.MarkEmsQueued(order->order_id); rc != ErrorCode::kSuccess) {
      account_risk_.Release(order->order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
      RecordLocalFailure(request, order->order_id, rc);
      spdlog::warn("[OrderIntentQueue] MarkEmsQueued failed order_id={} code={}", order->order_id, static_cast<int>(rc));
      return;
    }

    const auto rc = execution_.Enqueue(*order);
    if (rc != ErrorCode::kSuccess) {
      (void)orders_.RecordSendResult(order->order_id, rc);
      account_risk_.Release(order->order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
      spdlog::warn("[OrderIntentQueue] EMS Enqueue failed order_id={} code={}", order->order_id, static_cast<int>(rc));
      if (const auto latest = orders_.GetOrder(order->order_id); latest.has_value()) {
        NotifyOutcome(*latest);
      }
    }
  } catch (const std::exception& e) {
    spdlog::error("[OrderIntentQueue] Execute exception: {}", e.what());
  } catch (...) {
    spdlog::error("[OrderIntentQueue] Execute unknown exception");
  }
}

}  // namespace qtrade::engine
