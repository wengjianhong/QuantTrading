/// @file      compliance_manager.cpp
/// @brief     合规管理器实现
/// @details   实现交易合规检查，确保符合监管要求
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "compliance_manager.hpp"

#include <cmath>

namespace qtrade::engine::cms {

ErrorCode ComplianceManager::Configure(const ComplianceRules& rules) {
  // 1. 校验规则数值范围
  if (rules.min_volume <= 0 || rules.max_volume < rules.min_volume || rules.min_price < 0.0 || rules.max_price < 0.0 ||
      (rules.max_price > 0.0 && rules.max_price < rules.min_price) || rules.max_notional < 0.0) {
    return ErrorCode::kSystemError;
  }
  // 2. 拒绝版本回退后原子替换
  std::lock_guard lock(mutex_);
  if (rules.version != 0 && rules_.version > rules.version) {
    return ErrorCode::kSystemError;
  }
  rules_ = rules;
  return ErrorCode::kSuccess;
}

ErrorCode ComplianceManager::CheckOrder(const qtrade_sdk::trader::OrderRequest& request) const {
  // 1. 基础字段合法性
  if (request.instrument.empty() || !std::isfinite(request.price) || request.price < 0.0 ||
      request.side == qtrade_sdk::trader::SideType::kUnknown ||
      request.price_type == qtrade_sdk::trader::PriceType::kUnknown ||
      request.position_effect == qtrade_sdk::trader::PositionEffectType::kUnknown ||
      request.business_type == qtrade_sdk::trader::BusinessType::kUnknown) {
    return ErrorCode::kSystemError;
  }

  // 2. 拷贝当前规则快照后做限幅与白名单检查
  ComplianceRules rules;
  {
    std::lock_guard lock(mutex_);
    rules = rules_;
  }
  if (!rules.enabled) {
    return ErrorCode::kNotInitialized;
  }
  const double notional = request.price * static_cast<double>(request.volume);
  if (request.volume < rules.min_volume || request.volume > rules.max_volume ||
      (request.price_type == qtrade_sdk::trader::PriceType::kLimit && request.price <= 0.0) ||
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
