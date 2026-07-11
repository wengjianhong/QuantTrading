/// @file      execution_manager.hpp
/// @brief     交易执行管理器
/// @details   负责订单执行、报单管理以及成交回报处理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_

namespace qtrade::engine::ems {

class ExecutionManager {
 public:
  void Start();
  void Stop();
};

}  // namespace qtrade::engine::ems

#endif  // QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_
