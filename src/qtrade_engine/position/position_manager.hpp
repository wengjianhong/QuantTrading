/// @file      position_manager.hpp
/// @brief     持仓管理器（实现 PositionApi）
/// @details   按合约与官方持仓方向维护 qtrade::sdk::trader::Position；
///            支持柜台快照覆盖与成交幂等增量更新。
///            兄弟模块只依赖 PositionApi；柜台快照由组合根调用本类。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_POSITION_MANAGER_HPP_

#include "qtrade/engine/position/position_api.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace qtrade::engine::position {
using qtrade::sdk::trader::Position;
using qtrade::sdk::trader::PositionDirectionType;
using qtrade::sdk::trader::Trade;

/// @brief 按合约与方向维护持仓的幂等管理器
class PositionManager final : public PositionApi {
 public:
  /// @brief 构造空持仓视图
  PositionManager() = default;

  /// @brief 析构持仓管理器
  ~PositionManager() override = default;

  /// @brief 应用柜台持仓快照（组合根）
  /// @param positions 全量持仓列表（每条对应官方一个方向）
  void ApplyPositionSnapshot(const std::vector<Position>& positions);

  void ApplyTrade(const Trade& trade) override;
  [[nodiscard]] std::int64_t GetNetPosition(const std::string& instrument) const override;
  [[nodiscard]] std::int64_t GetGrossPosition(const std::string& instrument) const override;
  [[nodiscard]] std::optional<Position> GetPosition(const std::string& instrument,
                                                    PositionDirectionType direction) const override;
  [[nodiscard]] std::map<PositionDirectionType, Position> GetPositions(const std::string& instrument) const override;

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
