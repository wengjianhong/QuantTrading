/// @file      risk_manager.cpp
/// @brief     风险管理器实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/risk/risk_manager.hpp"

#include <cmath>
#include <utility>

namespace qtrade::engine::risk {

ErrorCode RiskManager::Configure(const RiskLimits& limits) {
  // 1. 校验预算参数
  if (limits.max_order_volume <= 0 || limits.max_order_notional < 0.0 || limits.max_total_notional < 0.0 ||
      limits.safety_buffer < 0.0 ||
      (limits.max_total_notional > 0.0 && limits.safety_buffer >= limits.max_total_notional)) {
    return ErrorCode::kSystemError;
  }
  // 2. 拒绝版本回退后原子替换
  std::lock_guard lock(mutex_);
  if (limits.version != 0 && limits_.version > limits.version) {
    return ErrorCode::kSystemError;
  }
  limits_ = limits;
  return ErrorCode::kSuccess;
}

void RiskManager::SetStateProviders(std::function<std::uint64_t()> open_orders_provider,
                                    std::function<double()> notional_provider) {
  std::lock_guard lock(mutex_);
  open_orders_provider_ = std::move(open_orders_provider);
  notional_provider_ = std::move(notional_provider);
}

std::uint64_t RiskManager::Version() const {
  std::lock_guard lock(mutex_);
  return limits_.version;
}

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

}  // namespace qtrade::engine::risk
