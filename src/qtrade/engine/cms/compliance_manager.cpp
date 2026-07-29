/// @file      compliance_manager.cpp
/// @brief     合规管理器实现
/// @details   实现交易合规检查，确保符合监管要求
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/cms/compliance_manager.hpp"

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


}  // namespace qtrade::engine::cms
