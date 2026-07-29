/// @file      execution_api.cpp
/// @brief     EMS ExecutionApi 接口实现（由 ExecutionManager 提供）
/// @details   仅实现 execution_api.hpp 中声明的模块间接口方法。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/ems/execution_manager.hpp"

namespace qtrade::engine::ems {

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

}  // namespace qtrade::engine::ems
