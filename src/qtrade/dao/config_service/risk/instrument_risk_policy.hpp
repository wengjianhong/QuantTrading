/// @file      instrument_risk_policy.hpp
/// @brief     instrument_risk_policy 表 DAO 声明（§3.2 品种级风控策略表）
/// @details   由 config-service 持久化并经 ConfigSnapshot 下发；引擎只读本地快照。当前仅提供 Record 与 DDL；DML
/// 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_INSTRUMENT_RISK_POLICY_HPP_
#define QTRADE_DAO_INSTRUMENT_RISK_POLICY_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief instrument_risk_policy 表行记录
/// @details 表主键说明：(tenant_id, account_id, engine_id, instrument_id)
///
struct InstrumentRiskPolicyRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 交易账户 ID
  std::optional<std::string> account_id;
  /// 引擎实例 ID；空串表示账户级缺省
  std::optional<std::string> engine_id;
  /// 合约/品种 ID
  std::optional<std::string> instrument_id;
  /// 限额配置版本
  std::optional<std::uint64_t> version;
  /// 单品种净仓上限
  std::optional<double> max_net_position;
  /// 单品种总仓上限
  std::optional<double> max_gross_position;
  /// 单笔最大数量
  std::optional<double> max_order_qty;
  /// 单笔最大名义金额
  std::optional<double> max_order_notional;
  /// 价格偏离上限（基点）
  std::optional<std::uint64_t> max_price_deviation_bps;
  /// 是否启用涨跌停校验
  std::optional<bool> limit_up_down_check_enabled;
  /// 流动性限制名义金额；0 表示不限制
  std::optional<double> liquidity_limit_notional;
  /// 是否启用品种级限额
  std::optional<bool> enabled;
};

/// @brief instrument_risk_policy 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class InstrumentRiskPolicy final : public ITableDdl {
 public:
  InstrumentRiskPolicy() = default;
  InstrumentRiskPolicy(InstrumentRiskPolicy&&) = delete;
  InstrumentRiskPolicy(const InstrumentRiskPolicy&) = delete;
  InstrumentRiskPolicy& operator=(InstrumentRiskPolicy&&) = delete;
  InstrumentRiskPolicy& operator=(const InstrumentRiskPolicy&) = delete;
  ~InstrumentRiskPolicy() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "config"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "instrument_risk_policy"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_INSTRUMENT_RISK_POLICY_HPP_
