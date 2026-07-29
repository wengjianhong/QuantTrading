/// @file      risk_api.cpp
/// @brief     实例风控 RiskApi 接口实现（由 RiskManager 提供）
/// @details   仅实现 risk_api.hpp 中声明的模块间接口方法。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/risk/risk_manager.hpp"

#include <cmath>

namespace qtrade::engine::risk {

ErrorCode RiskManager::CheckOrder(const qtrade_sdk::trader::OrderRequest& request) const {
  // 1. 基础字段合法性
  if (request.instrument.empty() || request.volume <= 0 || !std::isfinite(request.price) || request.price < 0.0 ||
      (request.price_type == qtrade_sdk::trader::PriceType::kLimit && request.price <= 0.0)) {
    return ErrorCode::kSystemError;
  }

  // 2. 拷贝预算与状态读取器
  RiskLimits limits;
  std::function<std::uint64_t()> open_orders_provider;
  std::function<double()> notional_provider;
  {
    std::lock_guard lock(mutex_);
    limits = limits_;
    open_orders_provider = open_orders_provider_;
    notional_provider = notional_provider_;
  }

  // 3. 单笔与累计敞口预算检查
  const double order_notional = request.price * static_cast<double>(request.volume);
  if (!std::isfinite(order_notional) || request.volume > limits.max_order_volume ||
      (limits.max_order_notional > 0.0 && order_notional > limits.max_order_notional)) {
    return ErrorCode::kResourceExhausted;
  }
  if (limits.max_open_orders > 0 && open_orders_provider && open_orders_provider() >= limits.max_open_orders) {
    return ErrorCode::kResourceExhausted;
  }
  if (limits.max_total_notional > 0.0 && notional_provider &&
      notional_provider() + order_notional > limits.max_total_notional - limits.safety_buffer) {
    return ErrorCode::kResourceExhausted;
  }
  return ErrorCode::kSuccess;
}

std::uint64_t RiskManager::Version() const {
  std::lock_guard lock(mutex_);
  return limits_.version;
}

}  // namespace qtrade::engine::risk
