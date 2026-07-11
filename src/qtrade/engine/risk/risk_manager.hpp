/// @file      risk_manager.hpp
/// @brief     风险管理器
/// @details   负责交易风险监控，包括仓位限制、盈亏控制等
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_

namespace qtrade::engine::risk {

class RiskManager {
 public:
  void Start();
  void Stop();
};

}  // namespace qtrade::engine::risk

#endif  // QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_
