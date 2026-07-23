/// @file      order_risk_policy.cpp
/// @brief     order_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/order_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS order_risk_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '交易账户 ID',
  engine_id TEXT NOT NULL COMMENT '引擎实例 ID',
  version BIGINT NOT NULL COMMENT '策略配置版本',
  min_qty DOUBLE NOT NULL COMMENT '单笔最小数量',
  max_qty DOUBLE NOT NULL COMMENT '单笔最大数量',
  min_price DOUBLE NOT NULL COMMENT '允许最低价；0 表示不限制',
  max_price DOUBLE NOT NULL COMMENT '允许最高价；0 表示不限制',
  allowed_sides_json TEXT NOT NULL COMMENT '允许方向 JSON 数组',
  allowed_order_types_json TEXT NOT NULL COMMENT '允许订单类型 JSON 数组',
  reject_duplicate_client_order_id BOOLEAN NOT NULL COMMENT '是否拒绝重复 client_order_id',
  max_order_ttl_ms BIGINT NOT NULL COMMENT '订单最大有效期（毫秒）；0 表示不限制',
  max_child_order_notional DOUBLE NOT NULL COMMENT '拆单子单累计名义上限；0 表示不限制',
  enabled BOOLEAN NOT NULL COMMENT '是否启用订单级策略',
  PRIMARY KEY (tenant_id, account_id, engine_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "config";

/// @brief 逻辑表名
const std::string kTableName = "order_risk_policy";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {};

}  // namespace

const std::string& OrderRiskPolicy::DatabaseName() const {
  return kDatabaseName;
}

const std::string& OrderRiskPolicy::TableName() const {
  return kTableName;
}

const std::vector<std::string>& OrderRiskPolicy::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& OrderRiskPolicy::GetIndexSqls() const {
  return kIndexSqls;
}

}  // namespace qtrade::framework::dao
