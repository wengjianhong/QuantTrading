/// @file      compliance_manager.cpp
/// @brief     合规规则执行器实现
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/compliance/compliance_manager.hpp"

namespace qtrade::engine::compliance {

ErrorCode ComplianceManager::CheckOrder(const qtrade::sdk::trader::OrderRequest&) const {
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::engine::compliance
