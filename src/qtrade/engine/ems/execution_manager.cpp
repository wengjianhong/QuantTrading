/// @file      execution_manager.cpp
/// @brief     交易执行管理器实现
/// @details   工作线程出队后调用 TraderApi 报单/撤单，并通过回调回写结果
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

void ExecutionManager::SetResultHandlers(BeforeSendHandler before_send,
                                         ResultHandler send_result,
                                         ResultHandler cancel_result) {
  std::lock_guard lock(mutex_);
  before_send_ = std::move(before_send);
  send_result_ = std::move(send_result);
  cancel_result_ = std::move(cancel_result);
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

// 工作线程主循环：出队 → before_send → SendOrder / CancelOrder → 回写结果
void ExecutionManager::Run() {
  while (true) {
    // 1. 等待出队并拷贝回调/通道指针
    WorkItem item;
    qtrade_sdk::trader::TraderApi* trader_api = nullptr;
    BeforeSendHandler before_send;
    ResultHandler send_result;
    ResultHandler cancel_result;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return !running_ || !pending_items_.empty(); });
      if (!running_ && pending_items_.empty()) {
        return;
      }
      item = std::move(pending_items_.front());
      pending_items_.pop_front();
      trader_api = trader_api_;
      before_send = before_send_;
      send_result = send_result_;
      cancel_result = cancel_result_;
    }

    // 2. 撤单直接调用通道
    if (item.type == WorkItem::Type::kCancel) {
      const auto result = trader_api != nullptr ? trader_api->CancelOrder(item.cancel) : ErrorCode::kNotInitialized;
      if (cancel_result) {
        cancel_result(item.cancel.order_id, result);
      }
      continue;
    }

    // 3. 新单先执行 before_send，失败则不发送
    const auto& order = item.order;
    if (before_send) {
      const auto result = before_send(order.order_id);
      if (result != ErrorCode::kSuccess) {
        if (send_result) {
          send_result(order.order_id, result);
        }
        continue;
      }
    }

    // 4. 组装 OrderRequest 并调用 SendOrder
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
    if (send_result) {
      send_result(order.order_id, result);
    }
  }
}

}  // namespace qtrade::engine::ems
