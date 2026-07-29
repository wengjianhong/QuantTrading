/// @file      execution_manager.cpp
/// @brief     交易执行管理器实现
/// @details   工作线程出队后调用 TraderApi 报单/撤单，并直接回写 OMS；发送失败时释放预占
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/ems/execution_manager.hpp"

#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/engine/oms/order_manager.hpp"

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

void ExecutionManager::SetOrderManager(oms::OrderManager* order_manager) {
  std::lock_guard lock(mutex_);
  order_manager_ = order_manager;
}

void ExecutionManager::SetAccountRiskClient(qtrade::client::AccountRiskClient* account_risk_client) {
  std::lock_guard lock(mutex_);
  account_risk_client_ = account_risk_client;
}

void ExecutionManager::SetAccountRiskIdentity(std::string tenant_id, std::string account_id) {
  std::lock_guard lock(mutex_);
  tenant_id_ = std::move(tenant_id);
  account_id_ = std::move(account_id);
}

void ExecutionManager::ReleaseReservationOnSendFailure(qtrade::client::AccountRiskClient* account_risk_client,
                                                       const std::string& tenant_id,
                                                       const std::string& account_id,
                                                       const std::string& order_id) {
  if (account_risk_client == nullptr || !account_risk_client->IsInitialized() || order_id.empty()) {
    return;
  }
  qtrade::account_risk::v1::ReleaseOrderRequest request;
  request.set_tenant_id(tenant_id);
  request.set_account_id(account_id);
  request.set_order_id(order_id);
  request.set_reason(qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
  qtrade::account_risk::v1::ReleaseOrderResponse response;
  if (const auto rc = account_risk_client->ReleaseOrder(request, response); rc != ErrorCode::kSuccess) {
    spdlog::warn("ReleaseOrder failed: order_id={}, code={}", order_id, static_cast<int>(rc));
  }
}

// 新单入队：FIFO 发送，队列满返回 kResourceExhausted
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

// 撤单入队：优先插队到队头，降低撤单延迟
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

// 工作线程主循环：出队 → MarkSendPending → SendOrder / CancelOrder → 回写 OMS
void ExecutionManager::Run() {
  while (true) {
    // 1. 等待出队并拷贝依赖指针/身份
    WorkItem item;
    qtrade_sdk::trader::TraderApi* trader_api = nullptr;
    oms::OrderManager* order_manager = nullptr;
    qtrade::client::AccountRiskClient* account_risk_client = nullptr;
    std::string tenant_id;
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
      order_manager = order_manager_;
      account_risk_client = account_risk_client_;
      tenant_id = tenant_id_;
      account_id = account_id_;
    }

    // 2. 撤单：调通道并回写 OMS
    if (item.type == WorkItem::Type::kCancel) {
      const auto result = trader_api != nullptr ? trader_api->CancelOrder(item.cancel) : ErrorCode::kNotInitialized;
      if (order_manager != nullptr) {
        (void)order_manager->RecordCancelResult(item.cancel.order_id, result);
      }
      continue;
    }

    // 3. 新单：发送前标记；失败则回写并释放预占，不调用通道
    const auto& order = item.order;
    if (order_manager != nullptr) {
      const auto pending_rc = order_manager->MarkSendPending(order.order_id);
      if (pending_rc != ErrorCode::kSuccess) {
        (void)order_manager->RecordSendResult(order.order_id, pending_rc);
        ReleaseReservationOnSendFailure(account_risk_client, tenant_id, account_id, order.order_id);
        continue;
      }
    }

    // 4. 组装 OrderRequest 并调用 SendOrder，回写 OMS；失败时释放预占
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
    const auto result = trader_api != nullptr ? trader_api->SendOrder(request) : ErrorCode::kNotInitialized;
    if (order_manager != nullptr) {
      (void)order_manager->RecordSendResult(order.order_id, result);
    }
    if (result != ErrorCode::kSuccess) {
      ReleaseReservationOnSendFailure(account_risk_client, tenant_id, account_id, order.order_id);
    }
  }
}

}  // namespace qtrade::engine::ems
