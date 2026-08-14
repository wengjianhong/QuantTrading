/// @file      account_api.hpp
/// @brief     账户资金视图对引擎内其他模块提供的稳定接口
/// @details   TraderEventHandler 等兄弟模块只依赖本接口，不依赖 AccountManager。
///            柜台资产快照由组合根通过 AccountManager 写入。
/// @author    wengjianhong
/// @date      2026-08-14
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ACCOUNT_ACCOUNT_API_HPP_
#define QTRADE_ENGINE_ACCOUNT_ACCOUNT_API_HPP_

#include <qtrade/sdk/trader/trader_struct.hpp>

namespace qtrade::engine::account {

/// @brief 账户资金模块间稳定接口（进程内；非 gRPC）
class AccountApi {
 public:
  virtual ~AccountApi() = default;

  /// @brief 根据订单剩余量更新本地冻结名义金额
  /// @param order 最新订单快照
  virtual void ApplyOrder(const qtrade::sdk::trader::Order& order) = 0;

  /// @brief 幂等应用成交回报并更新现金流
  /// @param trade 成交回报
  virtual void ApplyTrade(const qtrade::sdk::trader::Trade& trade) = 0;

  /// @brief 返回当前累计成交金额
  /// @return 已去重成交金额绝对值合计
  [[nodiscard]] virtual double GetFilledAmount() const = 0;

  /// @brief 返回本地订单冻结名义金额
  /// @return 活动买单 price × left_volume 合计
  [[nodiscard]] virtual double GetFrozenAmount() const = 0;

  /// @brief 返回成交净现金流
  /// @return 卖出为正、买入为负
  [[nodiscard]] virtual double GetNetCashFlow() const = 0;

  /// @brief 返回估算可用资金
  /// @return 柜台 buying_power + 成交净现金流 - 本地冻结
  [[nodiscard]] virtual double GetAvailableFunds() const = 0;
};

}  // namespace qtrade::engine::account

#endif  // QTRADE_ENGINE_ACCOUNT_ACCOUNT_API_HPP_
