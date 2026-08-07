/// @file      compliance_manager.hpp
/// @brief     合规管理器（实现 ComplianceApi）
/// @details   对下单请求做字段合法性与可配置白名单/限幅检查
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
#include <unordered_set>

namespace qtrade::engine::cms {

/// @brief 订单合规规则
struct ComplianceRules {
  /// 配置版本
  std::uint64_t version = 0;
  /// 是否启用下单
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
  std::unordered_set<qtrade_sdk::trader::SideType> allowed_sides;
  /// 允许价格类型；空集合表示全部已知类型
  std::unordered_set<qtrade_sdk::trader::PriceType> allowed_price_types;
};

/// @brief 订单字段与可配置白名单合规检查
class ComplianceManager final : public ComplianceApi {
 public:
  /// @brief 构造默认规则的合规管理器
  ComplianceManager() = default;

  /// @brief 析构合规管理器
  ~ComplianceManager() override = default;

  /// @brief 原子替换合规规则
  /// @param rules 新规则
  /// @return 版本回退或非法范围返回 kSystemError
  ErrorCode Configure(const ComplianceRules& rules);

  /// @brief 检查订单是否满足合规规则
  /// @param request 下单请求
  /// @return 通过返回 kSuccess
  [[nodiscard]] ErrorCode CheckOrder(const qtrade_sdk::trader::OrderRequest& request) const override;

 private:
  /// 保护规则快照
  mutable std::mutex mutex_;
  /// 当前规则
  ComplianceRules rules_;
};

}  // namespace qtrade::engine::cms

#endif  // QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
