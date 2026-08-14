/// @file      execution_manager.cpp
/// @brief     交易执行管理器实现
/// @details   工作线程出队后调用 TraderApi 报单/撤单，并直接回写 OMS；发送失败时经 AccountRiskApi 释放预占
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/ems/execution_manager.hpp"

#include "qtrade/engine/account_risk/account_risk_api.hpp"
#include "qtrade/engine/oms/order_api.hpp"

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
  pending_items_.clear();
}

void ExecutionManager::SetTraderApi(qtrade::sdk::trader::TraderApi* trader_api) {
  std::lock_guard lock(mutex_);
  trader_api_ = trader_api;
}

void ExecutionManager::SetOrderApi(oms::OrderApi* order_api) {
  std::lock_guard lock(mutex_);
  order_api_ = order_api;
}

void ExecutionManager::SetAccountRiskApi(account_risk::AccountRiskApi* account_risk) {
  std::lock_guard lock(mutex_);
  account_risk_ = account_risk;
}

ErrorCode ExecutionManager::Enqueue(const qtrade::sdk::trader::Order& order) {
  {
    std::lock_guard lock(mutex_);
    if (!running_ || !trader_api_) {
      return ErrorCode::kNotInitialized;
    }
    if (pending_items_.size() >= kQueueCapacity) {
      return ErrorCode::kResourceExhausted;
    }
    WorkItem item;
    item.type = WorkItem::Type::kSend;
    item.order = order;
    pending_items_.push_back(std::move(item));
  }
  cv_.notify_one();
  return ErrorCode::kSuccess;
}

ErrorCode ExecutionManager::EnqueueCancel(const qtrade::sdk::trader::CancelOrderRequest& request) {
  {
    std::lock_guard lock(mutex_);
    if (!running_ || !trader_api_) {
      return ErrorCode::kNotInitialized;
    }
    if (pending_items_.size() >= kQueueCapacity) {
      return ErrorCode::kResourceExhausted;
    }
    WorkItem item;
    item.type = WorkItem::Type::kCancel;
    item.cancel = request;
    pending_items_.push_front(std::move(item));
  }
  cv_.notify_one();
  return ErrorCode::kSuccess;
}

void ExecutionManager::Run() {
  while (true) {
    WorkItem item;
    qtrade::sdk::trader::TraderApi* trader_api = nullptr;
    oms::OrderApi* order_api = nullptr;
    account_risk::AccountRiskApi* account_risk = nullptr;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return !running_ || !pending_items_.empty(); });
      if (!running_ && pending_items_.empty()) {
        return;
      }
      item = std::move(pending_items_.front());
      pending_items_.pop_front();
      trader_api = trader_api_;
      order_api = order_api_;
      account_risk = account_risk_;
    }

    if (item.type == WorkItem::Type::kCancel) {
      const auto result = trader_api != nullptr ? trader_api->CancelOrder(item.cancel) : ErrorCode::kNotInitialized;
      if (order_api != nullptr) {
        (void)order_api->RecordCancelResult(item.cancel.order_id, result);
      }
      continue;
    }

    const auto& order = item.order;
    if (order_api != nullptr) {
      const auto pending_rc = order_api->MarkSendPending(order.order_id);
      if (pending_rc != ErrorCode::kSuccess) {
        (void)order_api->RecordSendResult(order.order_id, pending_rc);
        if (account_risk != nullptr) {
          account_risk->Release(order.order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
        }
        continue;
      }
    }

    qtrade::sdk::trader::OrderRequest request;
    request.client_order_id = order.client_order_id;
    request.broker_order_id = order.broker_order_id;
    request.instrument = order.instrument;
    request.market = order.market;
    request.price = order.price;
    request.volume = order.volume;
    request.price_type = order.price_type;
    request.side = order.side;
    request.position_effect = order.position_effect;
    request.business_type = order.business_type;
    const auto result = trader_api != nullptr ? trader_api->SendOrder(request) : ErrorCode::kNotInitialized;
    if (order_api != nullptr) {
      (void)order_api->RecordSendResult(order.order_id, result);
    }
    if (result != ErrorCode::kSuccess && account_risk != nullptr) {
      account_risk->Release(order.order_id, qtrade::account_risk::ReleaseReason::kSendFailed);
    }
  }
}

}  // namespace qtrade::engine::ems
