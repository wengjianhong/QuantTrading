/// @file      risk_manager.hpp
/// @brief     风险管理器
/// @details   负责交易风险监控，包括仓位限制、盈亏控制等
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <atomic>
#include <cstdint>
#include <limits>

namespace qtrade::engine::risk {

/// 订单风控检查的最小实现。
class RiskManager {
 public:
  RiskManager() = default;
  ~RiskManager() = default;

  void Start();
  void Stop();

  /// 检查订单参数及基础单笔数量限额。
  [[nodiscard]] ErrorCode CheckOrder(const qtrade_sdk::trader::OrderRequest& request) const;

 private:
  static constexpr std::int64_t kDefaultMaxOrderVolume = std::numeric_limits<std::int64_t>::max();

  std::atomic_bool running_ = false;
};

}  // namespace qtrade::engine::risk

#endif  // QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_
