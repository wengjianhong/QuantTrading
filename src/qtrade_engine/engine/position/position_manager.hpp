/// @file      position_manager.hpp
/// @brief     持仓管理器
/// @details   按合约与官方持仓方向维护 qtrade_sdk::trader::Position；
///            支持柜台快照覆盖与成交幂等增量更新。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_

#include <qtrade_sdk/trader/trader_struct.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace qtrade::engine::position {
using qtrade_sdk::trader::Position;
using qtrade_sdk::trader::PositionDirectionType;
using qtrade_sdk::trader::Trade;

/// @brief 按合约与方向维护持仓的幂等管理器
class PositionManager {
 public:
  /// @brief 构造空持仓视图
  PositionManager() = default;

  /// @brief 析构持仓管理器
  ~PositionManager() = default;

  /// @brief 幂等应用成交回报
  /// @param trade 成交回报
  void ApplyTrade(const Trade& trade);

  /// @brief 应用柜台持仓快照
  /// @param positions 全量持仓列表（每条对应官方一个方向）
  void ApplyPositionSnapshot(const std::vector<Position>& positions);

  /// @brief 返回指定合约的净持仓
  /// @param instrument 合约 ID
  /// @return 净持仓 = 多头数量 - 空头数量
  [[nodiscard]] std::int64_t GetNetPosition(const std::string& instrument) const;

  /// @brief 返回指定合约的总持仓
  /// @param instrument 合约 ID
  /// @return 总持仓 = 多头数量 + 空头数量
  [[nodiscard]] std::int64_t GetGrossPosition(const std::string& instrument) const;

  /// @brief 返回指定合约、指定方向的持仓
  /// @param instrument 合约 ID
  /// @param direction 官方持仓方向
  /// @return 存在则返回持仓快照
  [[nodiscard]] std::optional<Position> GetPosition(const std::string& instrument,
                                                    PositionDirectionType direction) const;

  /// @brief 返回指定合约下各方向持仓
  /// @param instrument 合约 ID
  /// @return 未跟踪合约返回空 map
  [[nodiscard]] std::map<PositionDirectionType, Position> GetPositions(const std::string& instrument) const;

 private:
  /// 保护持仓表与幂等集合的读写锁
  mutable std::shared_mutex mutex_;
  /// 已应用成交幂等键
  std::unordered_set<std::string> applied_trade_ids_;
  /// instrument → direction → 持仓（与柜台 OnQueryPosition 语义一致）
  std::map<std::string, std::map<PositionDirectionType, Position>> positions_;
};

}  // namespace qtrade::engine::position

#endif  // QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
