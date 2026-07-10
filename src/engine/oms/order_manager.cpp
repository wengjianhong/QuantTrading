/// @file      order_manager.cpp
/// @brief     订单管理器实现
/// @details   实现订单创建、修改、撤销及状态跟踪
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "engine/oms/order_manager.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

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
  std::lock_guard<std::mutex> lock(mutex_);
  if (!running_) {
    return ErrorCode::kNotInitialized;
  }

  const std::string order_id = "ORD-" + std::to_string(++order_id_counter_);

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
  spdlog::info("[OrderManager] order created: {}", order_id);
  return ErrorCode::kSuccess;
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

trader::Order* OrderManager::GetOrder(const std::string& order_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(order_id);
  if (it != orders_.end()) {
    return &it->second;
  }
  return nullptr;
}

void OrderManager::UpdateOrderStatus(const std::string& order_id, trader::OrderStatusType status) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(order_id);
  if (it != orders_.end()) {
    it->second.status = status;
    spdlog::info("[OrderManager] order {} status updated to {}", order_id, static_cast<int>(status));
  }
}

}  // namespace qtrade::engine::oms
