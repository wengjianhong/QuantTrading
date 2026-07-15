/// @file      risk_manager.cpp
/// @brief     风险管理器实现
/// @details   实现仓位限制、盈亏控制等风险监控逻辑
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/risk/risk_manager.hpp"

namespace qtrade::engine::risk {

void RiskManager::Start() {
  running_.store(true, std::memory_order_release);
}

void RiskManager::Stop() {
  running_.store(false, std::memory_order_release);
}

ErrorCode RiskManager::CheckOrder(const qtrade_sdk::trader::OrderRequest& request) const {
  if (!running_.load(std::memory_order_acquire)) {
    return ErrorCode::kNotInitialized;
  }
  if (request.instrument.empty() || request.volume <= 0 || request.price < 0.0 ||
      request.volume > kDefaultMaxOrderVolume) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::engine::risk
