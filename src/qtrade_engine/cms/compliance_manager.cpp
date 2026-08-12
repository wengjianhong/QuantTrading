/// @file      compliance_manager.cpp
/// @brief     合规管理器实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/cms/compliance_manager.hpp"

#include <cmath>

namespace qtrade::engine::cms {

ErrorCode ComplianceManager::UpsertStrategyRules(const std::string& strategy_id, const ComplianceRules& rules) {
  if (strategy_id.empty()) {
    return ErrorCode::kInvalidArgument;
  }
  if (rules.min_volume <= 0 || rules.max_volume < rules.min_volume || rules.min_price < 0.0 || rules.max_price < 0.0 ||
      (rules.max_price > 0.0 && rules.max_price < rules.min_price) || rules.max_notional < 0.0) {
    return ErrorCode::kSystemError;
  }
  std::lock_guard lock(mutex_);
  rules_by_strategy_[strategy_id] = rules;
  return ErrorCode::kSuccess;
}

void ComplianceManager::RemoveStrategyRules(const std::string& strategy_id) {
  if (strategy_id.empty()) {
    return;
  }
  std::lock_guard lock(mutex_);
  rules_by_strategy_.erase(strategy_id);
}

ErrorCode ComplianceManager::CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const {
  if (request.strategy_id.empty() || request.instrument.empty() || !std::isfinite(request.price) ||
      request.price < 0.0 || request.side == qtrade::sdk::trader::SideType::kUnknown ||
      request.price_type == qtrade::sdk::trader::PriceType::kUnknown ||
      request.position_effect == qtrade::sdk::trader::PositionEffectType::kUnknown ||
      request.business_type == qtrade::sdk::trader::BusinessType::kUnknown) {
    return ErrorCode::kInvalidArgument;
  }

  ComplianceRules rules;
  {
    std::lock_guard lock(mutex_);
    const auto it = rules_by_strategy_.find(request.strategy_id);
    if (it == rules_by_strategy_.end()) {
      return ErrorCode::kNotFound;
    }
    rules = it->second;
  }
  if (!rules.enabled) {
    return ErrorCode::kNotInitialized;
  }

  const double notional = request.price * static_cast<double>(request.volume);
  if (request.volume < rules.min_volume || request.volume > rules.max_volume ||
      (request.price_type == qtrade::sdk::trader::PriceType::kLimit && request.price <= 0.0) ||
      (rules.min_price > 0.0 && request.price < rules.min_price) ||
      (rules.max_price > 0.0 && request.price > rules.max_price) || !std::isfinite(notional) ||
      (rules.max_notional > 0.0 && notional > rules.max_notional) ||
      (!rules.allowed_instruments.empty() && !rules.allowed_instruments.contains(request.instrument)) ||
      (!rules.allowed_sides.empty() && !rules.allowed_sides.contains(request.side)) ||
      (!rules.allowed_price_types.empty() && !rules.allowed_price_types.contains(request.price_type))) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::engine::cms
