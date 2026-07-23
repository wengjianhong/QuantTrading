/// @file      position_manager.hpp
/// @brief     持仓管理器
/// @details   按合约维护多空今昨仓与均价；支持柜台快照覆盖与成交幂等增量更新
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_

#include <qtrade_sdk/trader/trader_struct.hpp>

#include <cstdint>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace qtrade::engine::position {

/// @brief 单合约多空持仓快照
struct PositionSnapshot {
  /// 多头今仓
  std::int64_t long_today = 0;
  /// 多头昨仓
  std::int64_t long_yesterday = 0;
  /// 空头今仓
  std::int64_t short_today = 0;
  /// 空头昨仓
  std::int64_t short_yesterday = 0;
  /// 多头平均开仓价
  double long_avg_price = 0.0;
  /// 空头平均开仓价
  double short_avg_price = 0.0;
};

/// @brief 按合约维护多空、今昨与净持仓的幂等管理器
class PositionManager {
 public:
  /// 成交回报别名
  using Trade = qtrade_sdk::trader::Trade;

  /// @brief 构造空持仓视图
  PositionManager() = default;

  /// @brief 析构持仓管理器
  ~PositionManager() = default;

  /// @brief 应用柜台持仓快照
  /// @param positions 全量持仓列表
  void ApplyPositionSnapshot(const std::vector<qtrade_sdk::trader::Position>& positions);

  /// @brief 幂等应用成交回报
  /// @param trade 成交回报
  void ApplyTrade(const Trade& trade);

  /// @brief 返回指定合约的净持仓
  /// @param instrument 合约 ID
  /// @return 多头减空头
  [[nodiscard]] std::int64_t GetNetPosition(const std::string& instrument) const;

  /// @brief 返回指定合约的总持仓
  /// @param instrument 合约 ID
  /// @return 多头加空头
  [[nodiscard]] std::int64_t GetGrossPosition(const std::string& instrument) const;

  /// @brief 返回指定合约完整持仓快照
  /// @param instrument 合约 ID
  /// @return 未跟踪合约返回零值快照
  [[nodiscard]] PositionSnapshot GetPosition(const std::string& instrument) const;

 private:
  /// 保护持仓表与幂等集合的读写锁
  mutable std::shared_mutex mutex_;
  /// instrument → 多空持仓
  std::unordered_map<std::string, PositionSnapshot> positions_;
  /// 已应用成交幂等键
  std::unordered_set<std::string> applied_trade_ids_;
};

}  // namespace qtrade::engine::position

#endif  // QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
