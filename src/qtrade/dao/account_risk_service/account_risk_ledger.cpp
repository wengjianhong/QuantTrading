/// @file      account_risk_ledger.cpp
/// @brief     account_risk_ledger 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/account_risk_service/account_risk_ledger.hpp"

namespace qtrade::framework::dao {
namespace {

constexpr const char* kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS account_risk_ledger (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '交易账户 ID',
  ledger_version BIGINT NOT NULL COMMENT '账簿版本；每次结算/预占变更递增',
  filled_notional DOUBLE NOT NULL COMMENT '已成交占用名义金额',
  filled_margin DOUBLE NOT NULL COMMENT '已成交占用保证金',
  reserved_notional DOUBLE NOT NULL COMMENT '已预占名义金额',
  reserved_margin DOUBLE NOT NULL COMMENT '已预占保证金',
  reserved_open_orders BIGINT NOT NULL COMMENT '已预占未完成订单数',
  gross_exposure DOUBLE NOT NULL COMMENT '当前总敞口',
  net_exposure DOUBLE NOT NULL COMMENT '当前净敞口',
  daily_pnl DOUBLE NOT NULL COMMENT '日内盈亏（亏损为负）',
  updated_at_unix_ms BIGINT NOT NULL COMMENT '最近更新时间（Unix 毫秒）',
  PRIMARY KEY (tenant_id, account_id)
);
)";

}  // namespace

AccountRiskLedger& AccountRiskLedger::Instance() {
  static AccountRiskLedger instance;
  return instance;
}

const std::string& AccountRiskLedger::TableName() const {
  static const std::string kName = "account_risk_ledger";
  return kName;
}

const std::vector<std::string>& AccountRiskLedger::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& AccountRiskLedger::GetIndexSqls() const {
  static const std::vector<std::string> kSqls = {
    R"(CREATE INDEX IF NOT EXISTS idx_account_risk_ledger_updated ON account_risk_ledger (updated_at_unix_ms);)"
  };
  return kSqls;
}

}  // namespace qtrade::framework::dao
