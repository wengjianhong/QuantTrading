/// @file      execution_manager.cpp
/// @brief     交易执行管理器实现
/// @details   工作线程出队后调用 TraderApi 报单/撤单，并直接回写 OMS；发送失败时释放预占
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/ems/execution_manager.hpp"

#include "qtrade/engine/oms/order_api.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::ems {

ExecutionManager::~ExecutionManager() {
  Stop();
}

void ExecutionManager::Start() {
  std::lock_guard lock(mutex_);
  if (running_) {
    return;
  }
  // 启动单线程工作队列，串行调用 TraderApi 避免通道并发写
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
  // 1. 唤醒工作线程并等待排空
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
  // 2. 丢弃未发送的待发队列
  std::lock_guard lock(mutex_);
  pending_items_.clear();
}

void ExecutionManager::SetTraderApi(qtrade_sdk::trader::TraderApi* trader_api) {
  std::lock_guard lock(mutex_);
  trader_api_ = trader_api;
}

void ExecutionManager::SetOrderApi(oms::OrderApi* order_api) {
  std::lock_guard lock(mutex_);
  order_api_ = order_api;
}

void ExecutionManager::SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* account_risk_bridge) {
  std::lock_guard lock(mutex_);
  account_risk_bridge_ = account_risk_bridge;
}

void ExecutionManager::SetAccountRiskIdentity(std::string account_id) {
  std::lock_guard lock(mutex_);
  account_id_ = std::move(account_id);
}

ErrorCode ExecutionManager::Enqueue(const qtrade_sdk::trader::Order& order) {
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

ErrorCode ExecutionManager::EnqueueCancel(const qtrade_sdk::trader::CancelOrderRequest& request) {
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

void ExecutionManager::ReleaseReservationOnSendFailure(qtrade::account_risk::IAccountRiskBridge* account_risk_bridge,
                                                       const std::string& account_id,
                                                       const std::string& order_id) {
  if (account_risk_bridge == nullptr || order_id.empty()) {
    return;
  }
  const auto result = account_risk_bridge->ReleaseOrder(
    account_id, order_id, qtrade::account_risk::ReleaseReason::kEmsEnqueueFailed, 0.0, 0.0);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("ReleaseOrder failed: order_id={}, code={}", order_id, static_cast<int>(result.error_code));
  }
}

// 工作线程主循环：出队 → MarkSendPending → SendOrder / CancelOrder → 回写 OMS
void ExecutionManager::Run() {
  while (true) {
    // 1. 等待出队并拷贝依赖指针/身份
    WorkItem item;
    qtrade_sdk::trader::TraderApi* trader_api = nullptr;
    oms::OrderApi* order_api = nullptr;
    qtrade::account_risk::IAccountRiskBridge* account_risk_bridge = nullptr;
    std::string account_id;
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
      account_risk_bridge = account_risk_bridge_;
      account_id = account_id_;
    }

    // 2. 撤单：调通道并回写 OMS
    if (item.type == WorkItem::Type::kCancel) {
      const auto result = trader_api != nullptr ? trader_api->CancelOrder(item.cancel) : ErrorCode::kNotInitialized;
      if (order_api != nullptr) {
        (void)order_api->RecordCancelResult(item.cancel.order_id, result);
      }
      continue;
    }

    // 3. 新单：发送前标记；失败则回写并释放预占，不调用通道
    const auto& order = item.order;
    if (order_api != nullptr) {
      const auto pending_rc = order_api->MarkSendPending(order.order_id);
      if (pending_rc != ErrorCode::kSuccess) {
        (void)order_api->RecordSendResult(order.order_id, pending_rc);
        ReleaseReservationOnSendFailure(account_risk_bridge, account_id, order.order_id);
        continue;
      }
    }

    // 4. 组装 OrderRequest 并调用 SendOrder，回写 OMS；失败时释放预占
    qtrade_sdk::trader::OrderRequest request;
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
    if (result != ErrorCode::kSuccess) {
      ReleaseReservationOnSendFailure(account_risk_bridge, account_id, order.order_id);
    }
  }
}

}  // namespace qtrade::engine::ems
