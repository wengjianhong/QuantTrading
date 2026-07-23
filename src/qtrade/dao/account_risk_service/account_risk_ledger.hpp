/// @file      account_risk_ledger.hpp
/// @brief     account_risk_ledger 表 DAO 声明（§3.2 账户权威账簿快照表（已成交 + 已预占））
/// @details   当前仅提供 Record 与 DDL；DML 待后续实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_ACCOUNT_RISK_LEDGER_HPP_
#define QTRADE_DAO_ACCOUNT_RISK_LEDGER_HPP_

#include <qtrade/dao/ddl.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief account_risk_ledger 表行记录
/// @details 表主键说明：(tenant_id, account_id)
///
struct AccountRiskLedgerRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 交易账户 ID
  std::optional<std::string> account_id;
  /// 账簿版本；每次结算/预占变更递增
  std::optional<std::uint64_t> ledger_version;
  /// 已成交占用名义金额
  std::optional<double> filled_notional;
  /// 已成交占用保证金
  std::optional<double> filled_margin;
  /// 已预占名义金额
  std::optional<double> reserved_notional;
  /// 已预占保证金
  std::optional<double> reserved_margin;
  /// 已预占未完成订单数
  std::optional<std::uint64_t> reserved_open_orders;
  /// 当前总敞口
  std::optional<double> gross_exposure;
  /// 当前净敞口
  std::optional<double> net_exposure;
  /// 日内盈亏（亏损为负）
  std::optional<double> daily_pnl;
  /// 最近更新时间（Unix 毫秒）
  std::optional<std::int64_t> updated_at_unix_ms;
};

/// @brief account_risk_ledger 表 DDL
/// @details 实现 ITableDdl；增删改查尚未实现
class AccountRiskLedger final : public ITableDdl {
 public:
  AccountRiskLedger() = default;
  AccountRiskLedger(AccountRiskLedger&&) = delete;
  AccountRiskLedger(const AccountRiskLedger&) = delete;
  AccountRiskLedger& operator=(AccountRiskLedger&&) = delete;
  AccountRiskLedger& operator=(const AccountRiskLedger&) = delete;
  ~AccountRiskLedger() noexcept override = default;

  /// @brief 获取逻辑数据库名
  /// @return 固定为 "account_risk"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "account_risk_ledger"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_ACCOUNT_RISK_LEDGER_HPP_
