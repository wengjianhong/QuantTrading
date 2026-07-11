/// @file      position_manager.hpp
/// @brief     持仓管理器
/// @details   负责持仓的实时管理，支持多账户、多策略持仓跟踪
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_

namespace qtrade::engine::position {

class PositionManager {
 public:
  void Start();
  void Stop();
};

}  // namespace qtrade::engine::position

#endif  // QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
