/// @file      instance_risk_policy.hpp
/// @brief     instance_risk_policy 表 DAO 声明（§3.2 实例级弱一致风险策略）
/// @details   主键 (tenant_id, account_id, engine_id)；对应 ConfigSnapshot 中的 RiskBudget。
///            由 config-service 持久化并下发；引擎只读本地快照。当前仅提供 Record 与 DDL；DML 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_INSTANCE_RISK_POLICY_HPP_
#define QTRADE_DAO_INSTANCE_RISK_POLICY_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief instance_risk_policy 表行记录
/// @details 表主键说明：(tenant_id, account_id, engine_id)
///
struct InstanceRiskPolicyRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 交易账户 ID
  std::optional<std::string> account_id;
  /// 引擎实例 ID
  std::optional<std::string> engine_id;
  /// 策略配置版本
  std::optional<std::uint64_t> version;
  /// 预算失效时间（Unix 毫秒）
  std::optional<std::int64_t> valid_until_unix_ms;
  /// 实例名义敞口预算
  std::optional<double> max_notional;
  /// 实例保证金预算
  std::optional<double> max_margin;
  /// 实例最大持仓名义预算
  std::optional<double> max_position_notional;
  /// 实例未完成订单数预算
  std::optional<std::uint64_t> max_open_orders;
  /// 实例订单速率预算（笔/秒）
  std::optional<std::uint64_t> max_order_rate_per_sec;
  /// 实例侧安全缓冲
  std::optional<double> safety_buffer;
  /// 是否启用实例预算校验
  std::optional<bool> enabled;
};

/// @brief instance_risk_policy 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class InstanceRiskPolicy final : public ITableDdl {
 public:
  InstanceRiskPolicy() = default;
  InstanceRiskPolicy(InstanceRiskPolicy&&) = delete;
  InstanceRiskPolicy(const InstanceRiskPolicy&) = delete;
  InstanceRiskPolicy& operator=(InstanceRiskPolicy&&) = delete;
  InstanceRiskPolicy& operator=(const InstanceRiskPolicy&) = delete;
  ~InstanceRiskPolicy() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "config"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "instance_risk_policy"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_INSTANCE_RISK_POLICY_HPP_
