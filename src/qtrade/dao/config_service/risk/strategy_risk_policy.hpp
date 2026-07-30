/// @file      strategy_risk_policy.hpp
/// @brief     strategy_risk_policy 表 DAO 声明（§3.2 策略级风控策略表）
/// @details   由 config-service 持久化并经 ConfigSnapshot 下发；引擎只读本地快照。当前仅提供 Record 与 DDL；DML
/// 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_STRATEGY_RISK_POLICY_HPP_
#define QTRADE_DAO_STRATEGY_RISK_POLICY_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief strategy_risk_policy 表行记录
/// @details 表主键说明：(tenant_id, account_id, engine_id, strategy_id)
///
struct StrategyRiskPolicyRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 允许交易的账户 ID
  std::optional<std::string> account_id;
  /// 引擎实例 ID
  std::optional<std::string> engine_id;
  /// 策略 ID
  std::optional<std::string> strategy_id;
  /// 限额配置版本
  std::optional<std::uint64_t> version;
  /// 策略资金预算
  std::optional<double> max_capital;
  /// 策略最大仓位（名义或数量口径由配置约定）
  std::optional<double> max_position_volume;
  /// 策略单日损失上限
  std::optional<double> max_daily_loss;
  /// 策略订单频率上限（笔/秒）
  std::optional<std::uint64_t> max_order_rate_per_sec;
  /// 策略撤单频率上限（笔/秒）
  std::optional<std::uint64_t> max_cancel_rate_per_sec;
  /// 允许品种 JSON 数组；空表示不额外限制
  std::optional<std::string> allowed_instruments_json;
  /// 是否启用策略级限额
  std::optional<bool> enabled;
};

/// @brief strategy_risk_policy 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class StrategyRiskPolicy final : public ITableDdl {
 public:
  StrategyRiskPolicy() = default;
  StrategyRiskPolicy(StrategyRiskPolicy&&) = delete;
  StrategyRiskPolicy(const StrategyRiskPolicy&) = delete;
  StrategyRiskPolicy& operator=(StrategyRiskPolicy&&) = delete;
  StrategyRiskPolicy& operator=(const StrategyRiskPolicy&) = delete;
  ~StrategyRiskPolicy() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "config"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "strategy_risk_policy"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_STRATEGY_RISK_POLICY_HPP_
