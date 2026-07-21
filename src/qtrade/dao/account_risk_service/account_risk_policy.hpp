/// @file      account_risk_policy.hpp
/// @brief     account_risk_policy 表 DAO 声明（§3.2 账户级硬风控策略表（E 段权威配置））
/// @details   当前仅提供 Record 与 DDL；DML 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_ACCOUNT_RISK_POLICY_HPP_
#define QTRADE_DAO_ACCOUNT_RISK_POLICY_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief account_risk_policy 表行记录
/// @details 表主键说明：(tenant_id, account_id)
///
struct AccountRiskPolicyRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 交易账户 ID
  std::optional<std::string> account_id;
  /// 策略版本；与 Reserve 的 risk_config_version 对齐
  std::optional<std::uint64_t> version;
  /// 策略失效时间（Unix 毫秒）；0 表示由服务端 TTL 决定
  std::optional<std::int64_t> valid_until_unix_ms;
  /// 账户名义金额硬上限
  std::optional<double> max_notional;
  /// 账户保证金占用硬上限
  std::optional<double> max_margin;
  /// 账户总敞口硬上限
  std::optional<double> max_gross_exposure;
  /// 账户净敞口硬上限
  std::optional<double> max_net_exposure;
  /// 未完成订单数硬上限
  std::optional<std::uint64_t> max_open_orders;
  /// 账户日内损失硬上限（绝对值）
  std::optional<double> max_daily_loss;
  /// 安全缓冲；实例预算之和须不超过硬上限减该值
  std::optional<double> safety_buffer;
  /// 是否启用账户级硬限制
  std::optional<bool> enabled;
};

/// @brief account_risk_policy 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class AccountRiskPolicy final : public ITableDdl {
 public:
  AccountRiskPolicy() = default;
  AccountRiskPolicy(AccountRiskPolicy&&) = delete;
  AccountRiskPolicy(const AccountRiskPolicy&) = delete;
  AccountRiskPolicy& operator=(AccountRiskPolicy&&) = delete;
  AccountRiskPolicy& operator=(const AccountRiskPolicy&) = delete;
  ~AccountRiskPolicy() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "account_risk"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "account_risk_policy"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_ACCOUNT_RISK_POLICY_HPP_
