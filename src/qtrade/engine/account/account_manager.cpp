/// @file      account_manager.cpp
/// @brief     账户管理器实现
/// @details   实现账户资金与持仓的查询与管理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "account_manager.hpp"

namespace qtrade::engine::account {

void AccountManager::Start() {
  running_.store(true, std::memory_order_release);
}

void AccountManager::Stop() {
  running_.store(false, std::memory_order_release);
}

void AccountManager::ApplyOrder(const Order& order) {
  if (!running_.load(std::memory_order_acquire)) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  const auto [it, inserted] = order_trade_amounts_.try_emplace(order.order_id, order.trade_amount);
  if (inserted) {
    filled_amount_ += order.trade_amount;
    return;
  }

  filled_amount_ += order.trade_amount - it->second;
  it->second = order.trade_amount;
}

void AccountManager::ApplyTrade(const Trade& trade) {
  if (!running_.load(std::memory_order_acquire)) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  filled_amount_ += trade.trade_amount;
}

double AccountManager::GetFilledAmount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return filled_amount_;
}

}  // namespace qtrade::engine::account
