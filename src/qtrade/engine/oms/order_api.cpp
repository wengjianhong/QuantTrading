/// @file      order_api.cpp
/// @brief     OMS OrderApi 接口实现（由 OrderManager 提供）
/// @details   仅实现 order_api.hpp 中声明的模块间接口方法。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/oms/order_manager.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::oms {
namespace trader = qtrade_sdk::trader;

std::string OrderManager::AllocateOrderId() {
  const auto sequence = order_id_counter_.fetch_add(1, std::memory_order_acq_rel) + 1;
  return tenant_id_ + "-" + engine_id_ + "-" + std::to_string(engine_epoch_) + "-" + std::to_string(sequence);
}

std::optional<trader::Order> OrderManager::CreateOrder(const trader::OrderRequest& request,
                                                       const std::string& order_id) {
  std::lock_guard lock(mutex_);
  if (!initialized_ || order_id.empty()) {
    return std::nullopt;
  }
  if (request.client_order_id != 0) {
    const auto existing = client_order_index_.find(request.client_order_id);
    if (existing != client_order_index_.end()) {
      return orders_.at(existing->second).order;
    }
  }

  OrderEntry entry;
  entry.order.order_id = order_id;
  entry.order.client_order_id = request.client_order_id;
  entry.order.instrument = request.instrument;
  entry.order.market = request.market;
  entry.order.price = request.price;
  entry.order.volume = request.volume;
  entry.order.left_volume = request.volume;
  entry.order.price_type = request.price_type;
  entry.order.side = request.side;
  entry.order.position_effect = request.position_effect;
  entry.order.business_type = request.business_type;
  entry.order.status = trader::OrderStatusType::kNew;
  entry.order.submit_status = trader::OrderSubmitStatusType::kInit;
  entry.lifecycle_state = OrderLifecycleState::kPrepared;

  orders_[order_id] = entry;
  if (request.client_order_id != 0) {
    client_order_index_[request.client_order_id] = order_id;
  }
  spdlog::info("[OrderManager] order created: {}", order_id);
  return entry.order;
}

ErrorCode OrderManager::MarkEmsQueued(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  return ApplyTransition(it->second, OrderLifecycleState::kEmsQueued);
}

ErrorCode OrderManager::MarkSendPending(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  it->second.order.submit_status = trader::OrderSubmitStatusType::kInsertSubmitted;
  return ApplyTransition(it->second, OrderLifecycleState::kSendPending);
}

ErrorCode OrderManager::RecordSendResult(const std::string& order_id, ErrorCode result) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }

  auto target_state = it->second.lifecycle_state;
  if (target_state == OrderLifecycleState::kSendPending ||
      (target_state == OrderLifecycleState::kEmsQueued && result != ErrorCode::kSuccess)) {
    target_state = OrderLifecycleState::kWorking;
    if (result == ErrorCode::kTimeout) {
      target_state = OrderLifecycleState::kSendUnknown;
      it->second.order.status = trader::OrderStatusType::kUnknown;
    } else if (result != ErrorCode::kSuccess) {
      target_state = OrderLifecycleState::kRejected;
      it->second.order.status = trader::OrderStatusType::kRejected;
      it->second.order.submit_status = trader::OrderSubmitStatusType::kInsertRejected;
    }
  }
  return ApplyTransition(it->second, target_state);
}

ErrorCode OrderManager::RecordCancelResult(const std::string& order_id, ErrorCode result) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  if (result == ErrorCode::kSuccess || result == ErrorCode::kTimeout) {
    return ApplyTransition(it->second, OrderLifecycleState::kCancelPending);
  }

  it->second.order.submit_status = trader::OrderSubmitStatusType::kCancelRejected;
  const auto fallback =
    it->second.order.traded_volume > 0 ? OrderLifecycleState::kPartiallyFilled : OrderLifecycleState::kWorking;
  return ApplyTransition(it->second, fallback);
}

std::optional<trader::Order> OrderManager::GetOrderByClientId(std::uint32_t client_order_id) const {
  std::lock_guard lock(mutex_);
  const auto index_it = client_order_index_.find(client_order_id);
  if (index_it == client_order_index_.end()) {
    return std::nullopt;
  }
  const auto order_it = orders_.find(index_it->second);
  return order_it == orders_.end() ? std::nullopt : std::optional<trader::Order>(order_it->second.order);
}

std::optional<OrderLifecycleState> OrderManager::GetLifecycleState(const std::string& order_id) const {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  return it == orders_.end() ? std::nullopt : std::optional<OrderLifecycleState>(it->second.lifecycle_state);
}

}  // namespace qtrade::engine::oms
