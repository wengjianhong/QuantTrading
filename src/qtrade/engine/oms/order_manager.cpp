/// @file      order_manager.cpp
/// @brief     订单管理器实现
/// @details   实现订单创建、修改、撤销及状态跟踪
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/oms/order_manager.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

namespace qtrade::engine::oms {

namespace trader = qtrade_sdk::trader;

OrderManager::OrderManager() : running_(false), order_id_counter_(0) {}

OrderManager::~OrderManager() {
  Stop();
}

void OrderManager::Start() {
  std::lock_guard<std::mutex> lock(mutex_);
  running_ = true;
  spdlog::info("[OrderManager] started");
}

void OrderManager::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  running_ = false;
  spdlog::info("[OrderManager] stopped");
}

ErrorCode OrderManager::SendOrder(const trader::OrderRequest& request) {
  return CreateOrder(request).has_value() ? ErrorCode::kSuccess : ErrorCode::kNotInitialized;
}

std::optional<trader::Order> OrderManager::CreateOrder(const trader::OrderRequest& request) {
  return CreateOrder(request, AllocateOrderId());
}

std::string OrderManager::AllocateOrderId() {
  const auto now =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  return "ORD-" + std::to_string(now) + "-" + std::to_string(++order_id_counter_);
}

std::optional<trader::Order> OrderManager::CreateOrder(const trader::OrderRequest& request,
                                                       const std::string& order_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_ || order_id.empty()) {
    return std::nullopt;
  }
  if (request.client_order_id != 0) {
    const auto existing = client_order_index_.find(request.client_order_id);
    if (existing != client_order_index_.end()) {
      return orders_.at(existing->second);
    }
  }

  trader::Order new_order;
  new_order.order_id = order_id;
  new_order.client_order_id = request.client_order_id;
  new_order.instrument = request.instrument;
  new_order.market = request.market;
  new_order.price = request.price;
  new_order.volume = request.volume;
  new_order.left_volume = request.volume;
  new_order.price_type = request.price_type;
  new_order.side = request.side;
  new_order.position_effect = request.position_effect;
  new_order.business_type = request.business_type;
  new_order.status = trader::OrderStatusType::kNew;
  new_order.submit_status = trader::OrderSubmitStatusType::kInsertSubmitted;

  orders_[order_id] = new_order;
  if (request.client_order_id != 0) {
    client_order_index_[request.client_order_id] = order_id;
  }
  spdlog::info("[OrderManager] order created: {}", order_id);
  return new_order;
}

ErrorCode OrderManager::CancelOrder(const std::string& order_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  it->second.status = trader::OrderStatusType::kCanceled;
  spdlog::info("[OrderManager] order canceled: {}", order_id);
  return ErrorCode::kSuccess;
}

std::optional<trader::Order> OrderManager::GetOrder(const std::string& order_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it != orders_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<trader::Order> OrderManager::GetOrderByClientId(std::uint32_t client_order_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto index_it = client_order_index_.find(client_order_id);
  if (index_it == client_order_index_.end()) {
    return std::nullopt;
  }
  const auto order_it = orders_.find(index_it->second);
  return order_it == orders_.end() ? std::nullopt : std::optional<trader::Order>(order_it->second);
}

void OrderManager::UpdateOrderStatus(const std::string& order_id, trader::OrderStatusType status) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(order_id);
  if (it != orders_.end()) {
    it->second.status = status;
    spdlog::info("[OrderManager] order {} status updated to {}", order_id, static_cast<int>(status));
  }
}

void OrderManager::ApplyOrderReport(const trader::Order& report) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(report.order_id);
  if (it == orders_.end() && report.client_order_id != 0) {
    const auto index_it = client_order_index_.find(report.client_order_id);
    if (index_it != client_order_index_.end()) {
      it = orders_.find(index_it->second);
    }
  }
  if (it == orders_.end()) {
    spdlog::warn("[OrderManager] ignored order report without local order, client_order_id={}", report.client_order_id);
    return;
  }

  trader::Order& local = it->second;
  local.order_emt_id = report.order_emt_id != 0 ? report.order_emt_id : local.order_emt_id;
  local.exchange_order_id = report.exchange_order_id.empty() ? local.exchange_order_id : report.exchange_order_id;
  local.status = report.status;
  local.submit_status = report.submit_status;
  local.traded_volume = report.traded_volume;
  local.left_volume = report.left_volume;
  local.trade_amount = report.trade_amount;
  local.error_message = report.error_message;
}

void OrderManager::ApplyTradeReport(const trader::Trade& report) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(report.order_id);
  if (it == orders_.end() && report.client_order_id != 0) {
    const auto index_it = client_order_index_.find(report.client_order_id);
    if (index_it != client_order_index_.end()) {
      it = orders_.find(index_it->second);
    }
  }
  if (it == orders_.end()) {
    spdlog::warn("[OrderManager] ignored trade report without local order, client_order_id={}", report.client_order_id);
    return;
  }

  trader::Order& local = it->second;
  local.traded_volume += report.volume;
  local.left_volume = std::max<std::int64_t>(0, local.volume - local.traded_volume);
  local.trade_amount += report.trade_amount;
  local.status = local.left_volume == 0 ? trader::OrderStatusType::kFilled : trader::OrderStatusType::kPartiallyFilled;
}

}  // namespace qtrade::engine::oms
