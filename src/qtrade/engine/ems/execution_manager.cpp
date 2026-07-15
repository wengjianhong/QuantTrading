/// @file      execution_manager.cpp
/// @brief     交易执行管理器实现
/// @details   实现订单执行、报单管理及成交回报处理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/ems/execution_manager.hpp"

namespace qtrade::engine::ems {

ExecutionManager::~ExecutionManager() {
  Stop();
}

void ExecutionManager::Start() {
  std::lock_guard lock(mutex_);
  if (running_) {
    return;
  }
  running_ = true;
  worker_ = std::thread([this] { Run(); });
}

void ExecutionManager::Stop() {
  {
    std::lock_guard lock(mutex_);
    if (!running_) {
      return;
    }
    running_ = false;
  }
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
  std::lock_guard lock(mutex_);
  pending_orders_.clear();
}

void ExecutionManager::SetTraderApi(qtrade_sdk::trader::TraderApi* trader_api) {
  std::lock_guard lock(mutex_);
  trader_api_ = trader_api;
}

ErrorCode ExecutionManager::Enqueue(const qtrade_sdk::trader::Order& order) {
  {
    std::lock_guard lock(mutex_);
    if (!running_ || !trader_api_) {
      return ErrorCode::kNotInitialized;
    }
    pending_orders_.push_back(order);
  }
  cv_.notify_one();
  return ErrorCode::kSuccess;
}

void ExecutionManager::Run() {
  while (true) {
    qtrade_sdk::trader::Order order;
    qtrade_sdk::trader::TraderApi* trader_api = nullptr;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return !running_ || !pending_orders_.empty(); });
      if (!running_ && pending_orders_.empty()) {
        return;
      }
      order = std::move(pending_orders_.front());
      pending_orders_.pop_front();
      trader_api = trader_api_;
    }

    qtrade_sdk::trader::OrderRequest request;
    request.client_order_id = order.client_order_id;
    request.order_emt_id = order.order_emt_id;
    request.instrument = order.instrument;
    request.market = order.market;
    request.price = order.price;
    request.volume = order.volume;
    request.price_type = order.price_type;
    request.side = order.side;
    request.position_effect = order.position_effect;
    request.business_type = order.business_type;
    (void)trader_api->SendOrder(request);
  }
}

}  // namespace qtrade::engine::ems
