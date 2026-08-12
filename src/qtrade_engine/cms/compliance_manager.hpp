/// @file      compliance_manager.hpp
/// @brief     合规管理器（实现 ComplianceApi）
/// @details   按策略实例管理合规规则；对下单请求做字段合法性与白名单/限幅检查
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_

#include "qtrade/engine/cms/compliance_api.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace qtrade::engine::cms {

/// @brief 单策略订单合规规则
struct ComplianceRules {
  /// 是否启用该策略下单
  bool enabled = true;
  /// 单笔最小数量
  std::int64_t min_volume = 1;
  /// 单笔最大数量
  std::int64_t max_volume = std::numeric_limits<std::int64_t>::max();
  /// 最低限价；0 表示不限制
  double min_price = 0.0;
  /// 最高限价；0 表示不限制
  double max_price = 0.0;
  /// 单笔最大名义金额；0 表示不限制
  double max_notional = 0.0;
  /// 允许合约；空集合表示全部
  std::unordered_set<std::string> allowed_instruments;
  /// 允许买卖方向；空集合表示全部已知方向
  std::unordered_set<qtrade::sdk::trader::SideType> allowed_sides;
  /// 允许价格类型；空集合表示全部已知类型
  std::unordered_set<qtrade::sdk::trader::PriceType> allowed_price_types;
};

/// @brief 按策略管理的订单字段与白名单合规检查
class ComplianceManager final : public ComplianceApi {
 public:
  ComplianceManager() = default;
  ~ComplianceManager() override = default;

  /// @brief 注册或替换指定策略的合规规则
  /// @param strategy_id 策略实例标识
  /// @param rules 新规则
  /// @return 非法参数返回 kInvalidArgument / kSystemError
  ErrorCode UpsertStrategyRules(const std::string& strategy_id, const ComplianceRules& rules);

  /// @brief 移除指定策略的合规规则
  /// @param strategy_id 策略实例标识
  void RemoveStrategyRules(const std::string& strategy_id);

  /// @brief 按 request.strategy_id 检查订单
  [[nodiscard]] ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const override;

 private:
  mutable std::mutex mutex_;
  /// strategy_id → 合规规则
  std::unordered_map<std::string, ComplianceRules> rules_by_strategy_;
};

}  // namespace qtrade::engine::cms

#endif  // QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
