/// @file      instrument_risk_policy.cpp
/// @brief     instrument_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/instrument_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS instrument_risk_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '交易账户 ID',
  engine_id TEXT NOT NULL COMMENT '引擎实例 ID；空串表示账户级缺省',
  instrument_id TEXT NOT NULL COMMENT '合约/品种 ID',
  version BIGINT NOT NULL COMMENT '限额配置版本',
  max_net_position DOUBLE NOT NULL COMMENT '单品种净仓上限',
  max_gross_position DOUBLE NOT NULL COMMENT '单品种总仓上限',
  max_order_qty DOUBLE NOT NULL COMMENT '单笔最大数量',
  max_order_notional DOUBLE NOT NULL COMMENT '单笔最大名义金额',
  max_price_deviation_bps BIGINT NOT NULL COMMENT '价格偏离上限（基点）',
  limit_up_down_check_enabled BOOLEAN NOT NULL COMMENT '是否启用涨跌停校验',
  liquidity_limit_notional DOUBLE NOT NULL COMMENT '流动性限制名义金额；0 表示不限制',
  enabled BOOLEAN NOT NULL COMMENT '是否启用品种级限额',
  PRIMARY KEY (tenant_id, account_id, engine_id, instrument_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "config";

/// @brief 逻辑表名
const std::string kTableName = "instrument_risk_policy";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {
  R"(CREATE INDEX IF NOT EXISTS idx_instrument_risk_policy_instr ON instrument_risk_policy (tenant_id, instrument_id);)"};

}  // namespace

const std::string& InstrumentRiskPolicy::DatabaseName() const {
  return kDatabaseName;
}

const std::string& InstrumentRiskPolicy::TableName() const {
  return kTableName;
}

const std::vector<std::string>& InstrumentRiskPolicy::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& InstrumentRiskPolicy::GetIndexSqls() const {
  return kIndexSqls;
}

}  // namespace qtrade::framework::dao
