/// @file      account_risk_policy.cpp
/// @brief     account_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/account_risk_service/account_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS account_risk_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '交易账户 ID',
  version BIGINT NOT NULL COMMENT '策略版本；与 Reserve 的 risk_config_version 对齐',
  valid_until_unix_ms BIGINT NOT NULL COMMENT '策略失效时间（Unix 毫秒）；0 表示由服务端 TTL 决定',
  max_notional DOUBLE NOT NULL COMMENT '账户名义金额硬上限',
  max_margin DOUBLE NOT NULL COMMENT '账户保证金占用硬上限',
  max_gross_exposure DOUBLE NOT NULL COMMENT '账户总敞口硬上限',
  max_net_exposure DOUBLE NOT NULL COMMENT '账户净敞口硬上限',
  max_open_orders BIGINT NOT NULL COMMENT '未完成订单数硬上限',
  max_daily_loss DOUBLE NOT NULL COMMENT '账户日内损失硬上限（绝对值）',
  safety_buffer DOUBLE NOT NULL COMMENT '安全缓冲；实例预算之和须不超过硬上限减该值',
  enabled BOOLEAN NOT NULL COMMENT '是否启用账户级硬限制',
  PRIMARY KEY (tenant_id, account_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "account_risk";

/// @brief 逻辑表名
const std::string kTableName = "account_risk_policy";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {
  R"(CREATE INDEX IF NOT EXISTS idx_account_risk_policy_enabled ON account_risk_policy (tenant_id, enabled);)"};

}  // namespace

const std::string& AccountRiskPolicy::DatabaseName() const {
  return kDatabaseName;
}

const std::string& AccountRiskPolicy::TableName() const {
  return kTableName;
}

const std::vector<std::string>& AccountRiskPolicy::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& AccountRiskPolicy::GetIndexSqls() const {
  return kIndexSqls;
}

}  // namespace qtrade::framework::dao
