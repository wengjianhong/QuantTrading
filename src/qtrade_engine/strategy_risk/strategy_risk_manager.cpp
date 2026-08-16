/// @file      strategy_risk_manager.cpp
/// @brief     合规管理器实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/strategy_risk/strategy_risk_manager.hpp"

#include <cmath>

namespace qtrade::engine::strategy_risk {

ErrorCode StrategyRiskManager::UpsertStrategyRules(const std::string& strategy_id,
                                                 const qtrade::strategy::StrategyRiskLimits& risk) {
  if (strategy_id.empty()) {
    return ErrorCode::kInvalidArgument;
  }
  if (risk.max_volume < 0 || risk.max_notional < 0.0 || risk.max_position_volume < 0 ||
      risk.order_cooldown_ms < 0) {
    return ErrorCode::kSystemError;
  }
  std::lock_guard lock(mutex_);
  rules_by_strategy_[strategy_id] = risk;
  return ErrorCode::kSuccess;
}

void StrategyRiskManager::RemoveStrategyRules(const std::string& strategy_id) {
  if (strategy_id.empty()) {
    return;
  }
  std::lock_guard lock(mutex_);
  rules_by_strategy_.erase(strategy_id);
}

ErrorCode StrategyRiskManager::CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const {
  if (request.strategy_id.empty() || request.instrument.empty() || request.volume <= 0 ||
      !std::isfinite(request.price) || request.price < 0.0 ||
      request.side == qtrade::sdk::trader::SideType::kUnknown ||
      request.price_type == qtrade::sdk::trader::PriceType::kUnknown ||
      request.position_effect == qtrade::sdk::trader::PositionEffectType::kUnknown ||
      request.business_type == qtrade::sdk::trader::BusinessType::kUnknown) {
    return ErrorCode::kInvalidArgument;
  }

  qtrade::strategy::StrategyRiskLimits risk;
  {
    std::lock_guard lock(mutex_);
    const auto it = rules_by_strategy_.find(request.strategy_id);
    if (it == rules_by_strategy_.end()) {
      return ErrorCode::kNotFound;
    }
    risk = it->second;
  }

  const double notional = request.price * static_cast<double>(request.volume);
  if ((risk.max_volume > 0 && request.volume > risk.max_volume) ||
      (request.price_type == qtrade::sdk::trader::PriceType::kLimit && request.price <= 0.0) ||
      !std::isfinite(notional) || (risk.max_notional > 0.0 && notional > risk.max_notional)) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::engine::strategy_risk
