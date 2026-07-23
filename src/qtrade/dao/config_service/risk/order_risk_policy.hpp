/// @file      order_risk_policy.hpp
/// @brief     order_risk_policy 表 DAO 声明（§3.2 订单级风控策略表）
/// @details   由 config-service 持久化并经 ConfigSnapshot 下发；引擎只读本地快照。当前仅提供 Record 与 DDL；DML
/// 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_ORDER_RISK_POLICY_HPP_
#define QTRADE_DAO_ORDER_RISK_POLICY_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief order_risk_policy 表行记录
/// @details 表主键说明：(tenant_id, account_id, engine_id)
///
struct OrderRiskPolicyRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 交易账户 ID
  std::optional<std::string> account_id;
  /// 引擎实例 ID
  std::optional<std::string> engine_id;
  /// 策略配置版本
  std::optional<std::uint64_t> version;
  /// 单笔最小数量
  std::optional<double> min_qty;
  /// 单笔最大数量
  std::optional<double> max_qty;
  /// 允许最低价；0 表示不限制
  std::optional<double> min_price;
  /// 允许最高价；0 表示不限制
  std::optional<double> max_price;
  /// 允许方向 JSON 数组
  std::optional<std::string> allowed_sides_json;
  /// 允许订单类型 JSON 数组
  std::optional<std::string> allowed_order_types_json;
  /// 是否拒绝重复 client_order_id
  std::optional<bool> reject_duplicate_client_order_id;
  /// 订单最大有效期（毫秒）；0 表示不限制
  std::optional<std::int64_t> max_order_ttl_ms;
  /// 拆单子单累计名义上限；0 表示不限制
  std::optional<double> max_child_order_notional;
  /// 是否启用订单级策略
  std::optional<bool> enabled;
};

/// @brief order_risk_policy 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class OrderRiskPolicy final : public ITableDdl {
 public:
  OrderRiskPolicy() = default;
  OrderRiskPolicy(OrderRiskPolicy&&) = delete;
  OrderRiskPolicy(const OrderRiskPolicy&) = delete;
  OrderRiskPolicy& operator=(OrderRiskPolicy&&) = delete;
  OrderRiskPolicy& operator=(const OrderRiskPolicy&) = delete;
  ~OrderRiskPolicy() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "config"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "order_risk_policy"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_ORDER_RISK_POLICY_HPP_
