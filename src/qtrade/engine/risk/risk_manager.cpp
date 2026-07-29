/// @file      risk_manager.cpp
/// @brief     风险管理器实现
/// @details   实现仓位限制、盈亏控制等风险监控逻辑
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/risk/risk_manager.hpp"

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

}  // namespace qtrade::engine::risk
