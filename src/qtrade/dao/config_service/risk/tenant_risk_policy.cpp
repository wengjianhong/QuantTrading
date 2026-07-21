/// @file      tenant_risk_policy.cpp
/// @brief     tenant_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/tenant_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS tenant_risk_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  version BIGINT NOT NULL COMMENT '配额配置版本',
  valid_until_unix_ms BIGINT NOT NULL COMMENT '失效时间（Unix 毫秒）；0 表示不限期',
  max_notional DOUBLE NOT NULL COMMENT '租户总名义敞口上限',
  max_daily_loss DOUBLE NOT NULL COMMENT '租户总日内损失上限（绝对值）',
  max_order_rate_per_sec BIGINT NOT NULL COMMENT '租户总订单速率上限（笔/秒）',
  max_strategy_count BIGINT NOT NULL COMMENT '策略实例资源配额上限',
  max_engine_count BIGINT NOT NULL COMMENT '引擎实例资源配额上限',
  enabled BOOLEAN NOT NULL COMMENT '是否启用租户级硬限制',
  PRIMARY KEY (tenant_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "config";

/// @brief 逻辑表名
const std::string kTableName = "tenant_risk_policy";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {
  R"(CREATE INDEX IF NOT EXISTS idx_tenant_risk_policy_enabled ON tenant_risk_policy (enabled);)"};

}  // namespace

const std::string& TenantRiskPolicy::DatabaseName() const {
  return kDatabaseName;
}

const std::string& TenantRiskPolicy::TableName() const {
  return kTableName;
}

const std::vector<std::string>& TenantRiskPolicy::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& TenantRiskPolicy::GetIndexSqls() const {
  return kIndexSqls;
}

}  // namespace qtrade::framework::dao
