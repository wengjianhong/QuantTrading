/// @file      compliance_manager.cpp
/// @brief     合规管理器实现
/// @details   实现交易合规检查，确保符合监管要求
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "compliance_manager.hpp"

namespace qtrade::engine::cms {

void ComplianceManager::Start() {
  running_.store(true, std::memory_order_release);
}

void ComplianceManager::Stop() {
  running_.store(false, std::memory_order_release);
}

ErrorCode ComplianceManager::CheckOrder(const qtrade_sdk::trader::OrderRequest& request) const {
  if (!running_.load(std::memory_order_acquire)) {
    return ErrorCode::kNotInitialized;
  }
  if (request.instrument.empty() || request.volume <= 0 || request.price < 0.0) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::engine::cms
