/// @file      position_api.hpp
/// @brief     持仓视图对引擎内其他模块提供的稳定接口
/// @details   LaneEventHandler 等兄弟模块只依赖本接口，不依赖 PositionManager。
///            柜台持仓快照由组合根通过 PositionManager 写入。
/// @author    wengjianhong
/// @date      2026-08-14
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_POSITION_POSITION_API_HPP_
#define QTRADE_ENGINE_POSITION_POSITION_API_HPP_

#include <qtrade/sdk/trader/trader_struct.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace qtrade::engine::positions {

/// @brief 持仓模块间稳定接口（进程内；非 gRPC）
class PositionApi {
 public:
  /// @brief 析构持仓接口
  virtual ~PositionApi() = default;

  /// @brief 幂等应用成交回报
  /// @param trade 成交回报
  virtual void ApplyTrade(const qtrade::sdk::trader::Trade& trade) = 0;

  /// @brief 返回指定合约的净持仓
  /// @param instrument 合约 ID
  /// @return 净持仓 = 多头数量 - 空头数量
  [[nodiscard]] virtual std::int64_t GetNetPosition(const std::string& instrument) const = 0;

  /// @brief 返回指定合约的总持仓
  /// @param instrument 合约 ID
  /// @return 总持仓 = 多头数量 + 空头数量
  [[nodiscard]] virtual std::int64_t GetGrossPosition(const std::string& instrument) const = 0;

  /// @brief 返回指定合约、指定方向的持仓
  /// @param instrument 合约 ID
  /// @param direction 官方持仓方向
  /// @return 存在则返回持仓快照
  [[nodiscard]] virtual std::optional<qtrade::sdk::trader::Position> GetPosition(
    const std::string& instrument, qtrade::sdk::trader::PositionDirectionType direction) const = 0;

  /// @brief 返回指定合约下各方向持仓
  /// @param instrument 合约 ID
  /// @return 未跟踪合约返回空 map
  [[nodiscard]] virtual std::map<qtrade::sdk::trader::PositionDirectionType, qtrade::sdk::trader::Position>
  GetPositions(const std::string& instrument) const = 0;
};

}  // namespace qtrade::engine::positions

#endif  // QTRADE_ENGINE_POSITION_POSITION_API_HPP_
