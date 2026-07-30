/// @file      strategy_risk_policy.cpp
/// @brief     strategy_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/strategy_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS strategy_risk_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '允许交易的账户 ID',
  engine_id TEXT NOT NULL COMMENT '引擎实例 ID',
  strategy_id TEXT NOT NULL COMMENT '策略 ID',
  version BIGINT NOT NULL COMMENT '限额配置版本',
  max_capital DOUBLE NOT NULL COMMENT '策略资金预算',
  max_position_volume DOUBLE NOT NULL COMMENT '策略最大仓位（名义或数量口径由配置约定）',
  max_daily_loss DOUBLE NOT NULL COMMENT '策略单日损失上限',
  max_order_rate_per_sec BIGINT NOT NULL COMMENT '策略订单频率上限（笔/秒）',
  max_cancel_rate_per_sec BIGINT NOT NULL COMMENT '策略撤单频率上限（笔/秒）',
  allowed_instruments_json TEXT NOT NULL COMMENT '允许品种 JSON 数组；空表示不额外限制',
  enabled BOOLEAN NOT NULL COMMENT '是否启用策略级限额',
  PRIMARY KEY (tenant_id, account_id, engine_id, strategy_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "config";

/// @brief 逻辑表名
const std::string kTableName = "strategy_risk_policy";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {
  R"(CREATE INDEX IF NOT EXISTS idx_strategy_risk_policy_strategy ON strategy_risk_policy (tenant_id, strategy_id);)"};

}  // namespace

const std::string& StrategyRiskPolicy::DatabaseName() const {
  return kDatabaseName;
}

const std::string& StrategyRiskPolicy::TableName() const {
  return kTableName;
}

const std::vector<std::string>& StrategyRiskPolicy::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& StrategyRiskPolicy::GetIndexSqls() const {
  return kIndexSqls;
}

}  // namespace qtrade::framework::dao
