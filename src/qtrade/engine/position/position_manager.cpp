/// @file      position_manager.cpp
/// @brief     持仓管理器实现
/// @details   实现多账户、多策略持仓的实时跟踪与管理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/position/position_manager.hpp"

#include <mutex>

namespace qtrade::engine::position {

void PositionManager::Start() {
  running_.store(true, std::memory_order_release);
}

void PositionManager::Stop() {
  running_.store(false, std::memory_order_release);
}

void PositionManager::ApplyTrade(const Trade& trade) {
  if (!running_.load(std::memory_order_acquire) || trade.instrument.empty()) {
    return;
  }

  std::int64_t position_delta = 0;
  if (trade.side == qtrade_sdk::trader::SideType::kBuy) {
    position_delta = trade.volume;
  } else if (trade.side == qtrade_sdk::trader::SideType::kSell) {
    position_delta = -trade.volume;
  } else {
    return;
  }

  std::unique_lock<std::shared_mutex> lock(mutex_);
  net_positions_[trade.instrument] += position_delta;
}

std::int64_t PositionManager::GetNetPosition(const std::string& instrument) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  const auto it = net_positions_.find(instrument);
  return it == net_positions_.end() ? 0 : it->second;
}

}  // namespace qtrade::engine::position
