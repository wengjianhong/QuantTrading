/// @file      compliance_api.cpp
/// @brief     CMS ComplianceApi 接口实现（由 ComplianceManager 提供）
/// @details   仅实现 compliance_api.hpp 中声明的模块间接口方法。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/cms/compliance_manager.hpp"

#include <cmath>

namespace qtrade::engine::cms {

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
