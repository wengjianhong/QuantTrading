/// @file      position_manager.hpp
/// @brief     持仓管理器
/// @details   负责持仓的实时管理，支持多账户、多策略持仓跟踪
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_

#include <qtrade_sdk/trader/trader_struct.hpp>

#include <atomic>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace qtrade::engine::position {

/// 按合约维护净持仓的最小持仓管理器。
class PositionManager {
 public:
  using Trade = qtrade_sdk::trader::Trade;

  PositionManager() = default;
  ~PositionManager() = default;

  void Start();
  void Stop();

  /// 将成交回报应用到对应合约的净持仓。
  void ApplyTrade(const Trade& trade);

  /// 返回指定合约的净持仓；未跟踪合约返回零。
  [[nodiscard]] std::int64_t GetNetPosition(const std::string& instrument) const;

 private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, std::int64_t> net_positions_;
  std::atomic_bool running_ = false;
};

}  // namespace qtrade::engine::position

#endif  // QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
