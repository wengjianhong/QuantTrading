/// @file      instance_risk_policy.cpp
/// @brief     instance_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/instance_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS instance_risk_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '交易账户 ID',
  engine_id TEXT NOT NULL COMMENT '引擎实例 ID',
  version BIGINT NOT NULL COMMENT '策略配置版本',
  valid_until_unix_ms BIGINT NOT NULL COMMENT '预算失效时间（Unix 毫秒）',
  max_notional DOUBLE NOT NULL COMMENT '实例名义敞口预算',
  max_margin DOUBLE NOT NULL COMMENT '实例保证金预算',
  max_position_notional DOUBLE NOT NULL COMMENT '实例最大持仓名义预算',
  max_open_orders BIGINT NOT NULL COMMENT '实例未完成订单数预算',
  max_order_rate_per_sec BIGINT NOT NULL COMMENT '实例订单速率预算（笔/秒）',
  safety_buffer DOUBLE NOT NULL COMMENT '实例侧安全缓冲',
  enabled BOOLEAN NOT NULL COMMENT '是否启用实例预算校验',
  PRIMARY KEY (tenant_id, account_id, engine_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "config";

/// @brief 逻辑表名
const std::string kTableName = "instance_risk_policy";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {
  R"(CREATE INDEX IF NOT EXISTS idx_instance_risk_policy_engine ON instance_risk_policy (tenant_id, engine_id);)"};

}  // namespace

const std::string& InstanceRiskPolicy::DatabaseName() const {
  return kDatabaseName;
}

const std::string& InstanceRiskPolicy::TableName() const {
  return kTableName;
}

const std::vector<std::string>& InstanceRiskPolicy::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& InstanceRiskPolicy::GetIndexSqls() const {
  return kIndexSqls;
}

}  // namespace qtrade::framework::dao
