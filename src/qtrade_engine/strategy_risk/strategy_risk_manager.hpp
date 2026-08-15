/// @file      compliance_manager.hpp
/// @brief     合规管理器（实现 StrategyRiskApi）
/// @details   按 strategy_id 索引 StrategyRiskLimits；未登记的策略拒单
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_

#include "qtrade/engine/strategy_risk/strategy_risk_api.hpp"

#include <qtrade/strategy/strategy.hpp>

#include <mutex>
#include <string>
#include <unordered_map>

namespace qtrade::engine::strategy_risk {

/// @brief 按策略管理的限额合规检查（是否启用由 AddStrategy 是否登记决定）
class StrategyRiskManager final : public StrategyRiskApi {
 public:
  StrategyRiskManager() = default;
  ~StrategyRiskManager() override = default;

  /// @brief 注册或替换指定策略的限额规则
  /// @param strategy_id 策略实例标识
  /// @param risk 策略级限额（与 StrategyConfig.risk 同源）
  /// @return 非法参数返回 kInvalidArgument / kSystemError
  ErrorCode UpsertStrategyRules(const std::string& strategy_id, const qtrade::strategy::StrategyRiskLimits& risk);

  /// @brief 移除指定策略的合规规则
  /// @param strategy_id 策略实例标识
  void RemoveStrategyRules(const std::string& strategy_id);

  /// @brief 按 request.strategy_id 检查订单
  [[nodiscard]] ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const override;

 private:
  mutable std::mutex mutex_;
  /// strategy_id → 策略限额
  std::unordered_map<std::string, qtrade::strategy::StrategyRiskLimits> rules_by_strategy_;
};

}  // namespace qtrade::engine::strategy_risk

#endif  // QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
