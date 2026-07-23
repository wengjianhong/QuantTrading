/// @file      tenant_risk_policy.hpp
/// @brief     tenant_risk_policy 表 DAO 声明（§3.2 租户级风控策略表）
/// @details   由 config-service 持久化并经 ConfigSnapshot 下发；引擎只读本地快照。当前仅提供 Record 与 DDL；DML
/// 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_TENANT_RISK_POLICY_HPP_
#define QTRADE_DAO_TENANT_RISK_POLICY_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief tenant_risk_policy 表行记录
/// @details 表主键说明：(tenant_id)
///
struct TenantRiskPolicyRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 配额配置版本
  std::optional<std::uint64_t> version;
  /// 失效时间（Unix 毫秒）；0 表示不限期
  std::optional<std::int64_t> valid_until_unix_ms;
  /// 租户总名义敞口上限
  std::optional<double> max_notional;
  /// 租户总日内损失上限（绝对值）
  std::optional<double> max_daily_loss;
  /// 租户总订单速率上限（笔/秒）
  std::optional<std::uint64_t> max_order_rate_per_sec;
  /// 策略实例资源配额上限
  std::optional<std::uint64_t> max_strategy_count;
  /// 引擎实例资源配额上限
  std::optional<std::uint64_t> max_engine_count;
  /// 是否启用租户级硬限制
  std::optional<bool> enabled;
};

/// @brief tenant_risk_policy 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class TenantRiskPolicy final : public ITableDdl {
 public:
  TenantRiskPolicy() = default;
  TenantRiskPolicy(TenantRiskPolicy&&) = delete;
  TenantRiskPolicy(const TenantRiskPolicy&) = delete;
  TenantRiskPolicy& operator=(TenantRiskPolicy&&) = delete;
  TenantRiskPolicy& operator=(const TenantRiskPolicy&) = delete;
  ~TenantRiskPolicy() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "config"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "tenant_risk_policy"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_TENANT_RISK_POLICY_HPP_
