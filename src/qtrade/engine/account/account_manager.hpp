/// @file      account_manager.hpp
/// @brief     账户管理器
/// @details   负责账户资金、持仓的查询与管理
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_

#include <qtrade_sdk/trader/trader_struct.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

namespace qtrade::engine::account {

/// 维护账户成交金额的最小账户管理器。
class AccountManager {
 public:
  using Order = qtrade_sdk::trader::Order;
  using Trade = qtrade_sdk::trader::Trade;

  AccountManager() = default;
  ~AccountManager() = default;

  void Start();
  void Stop();

  /// 根据订单累计成交额更新账户状态。
  void ApplyOrder(const Order& order);

  /// 根据成交回报累计成交金额。
  void ApplyTrade(const Trade& trade);

  /// 返回当前累计成交金额。
  [[nodiscard]] double GetFilledAmount() const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, double> order_trade_amounts_;
  double filled_amount_ = 0.0;
  std::atomic_bool running_{false};
};

}  // namespace qtrade::engine::account

#endif  // QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_
