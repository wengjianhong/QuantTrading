/// @file      account_manager.hpp
/// @brief     账户管理器
/// @details   维护柜台资金快照、本地买单冻结与成交净现金流，供可用资金估算
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_

#include <qtrade/sdk/trader/trader_struct.hpp>

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace qtrade::engine::account {

/// @brief 引擎内账户资金、冻结与成交现金流视图
class AccountManager {
 public:
  /// 订单快照别名
  using Order = qtrade_sdk::trader::Order;
  /// 成交回报别名
  using Trade = qtrade_sdk::trader::Trade;

  /// @brief 构造空账户视图
  AccountManager() = default;

  /// @brief 析构账户管理器
  ~AccountManager() = default;

  /// @brief 应用柜台资金快照
  /// @param asset 最新账户资产
  void ApplyAssetSnapshot(const qtrade_sdk::trader::AccountAsset& asset);

  /// @brief 根据订单剩余量更新本地冻结名义金额
  /// @param order 最新订单快照
  void ApplyOrder(const Order& order);

  /// @brief 幂等应用成交回报并更新现金流
  /// @param trade 成交回报
  void ApplyTrade(const Trade& trade);

  /// @brief 返回当前累计成交金额
  /// @return 已去重成交金额绝对值合计
  [[nodiscard]] double GetFilledAmount() const;

  /// @brief 返回本地订单冻结名义金额
  /// @return 活动买单 price × left_volume 合计
  [[nodiscard]] double GetFrozenAmount() const;

  /// @brief 返回成交净现金流
  /// @return 卖出为正、买入为负
  [[nodiscard]] double GetNetCashFlow() const;

  /// @brief 返回估算可用资金
  /// @return 柜台 buying_power + 成交净现金流 - 本地冻结
  [[nodiscard]] double GetAvailableFunds() const;

 private:
  /// 互斥锁
  mutable std::mutex mutex_;
  /// order_id → 当前冻结名义金额；用于计算订单冻结名义金额
  std::unordered_map<std::string, double> order_frozen_amounts_;
  /// 已应用成交幂等键；用于避免重复应用成交
  std::unordered_set<std::string> applied_trade_ids_;
  /// 最近柜台资产快照；用于计算可用资金
  qtrade_sdk::trader::AccountAsset asset_;
  /// 是否已有柜台资产快照；用于避免重复应用资产快照
  bool has_asset_snapshot_ = false;
  /// 活动订单冻结合计；用于计算可用资金
  double frozen_amount_ = 0.0;
  /// 成交净现金流；用于计算可用资金
  double net_cash_flow_ = 0.0;
  /// 成交金额绝对值合计；用于计算可用资金
  double filled_amount_ = 0.0;
};

}  // namespace qtrade::engine::account

#endif  // QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_
