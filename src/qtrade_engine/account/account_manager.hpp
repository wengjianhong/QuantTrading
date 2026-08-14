/// @file      account_manager.hpp
/// @brief     账户管理器（实现 AccountApi）
/// @details   维护柜台资金快照、本地买单冻结与成交净现金流，供可用资金估算。
///            兄弟模块只依赖 AccountApi；柜台快照由组合根调用本类。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_ACCOUNT_MANAGER_HPP_

#include "qtrade/engine/account/account_api.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace qtrade::engine::account {

/// @brief 引擎内账户资金、冻结与成交现金流视图
class AccountManager final : public AccountApi {
 public:
  /// 订单快照别名
  using Order = qtrade::sdk::trader::Order;
  /// 成交回报别名
  using Trade = qtrade::sdk::trader::Trade;

  /// @brief 构造空账户视图
  AccountManager() = default;

  /// @brief 析构账户管理器
  ~AccountManager() override = default;

  /// @brief 应用柜台资金快照（组合根）
  /// @param asset 最新账户资产
  void ApplyAssetSnapshot(const qtrade::sdk::trader::AccountAsset& asset);

  void ApplyOrder(const Order& order) override;
  void ApplyTrade(const Trade& trade) override;
  [[nodiscard]] double GetFilledAmount() const override;
  [[nodiscard]] double GetFrozenAmount() const override;
  [[nodiscard]] double GetNetCashFlow() const override;
  [[nodiscard]] double GetAvailableFunds() const override;

 private:
  /// 互斥锁
  mutable std::mutex mutex_;
  /// order_id → 当前冻结名义金额；用于计算订单冻结名义金额
  std::unordered_map<std::string, double> order_frozen_amounts_;
  /// 已应用成交幂等键；用于避免重复应用成交
  std::unordered_set<std::string> applied_trade_ids_;
  /// 最近柜台资产快照；用于计算可用资金
  qtrade::sdk::trader::AccountAsset asset_;
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
