/// @file      account_manager.hpp
/// @brief     账户管理器
/// @details   负责账户资金、持仓的查询与管理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_

namespace qtrade::engine::account {

class AccountManager {
 public:
  void Start();
  void Stop();
};

}  // namespace qtrade::engine::account

#endif  // QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_
