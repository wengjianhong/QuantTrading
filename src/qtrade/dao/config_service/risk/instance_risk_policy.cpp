/// @file      instance_risk_policy.cpp
/// @brief     instance_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/instance_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

constexpr const char* kCreateTableSql = R"(
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

}  // namespace

InstanceRiskPolicy& InstanceRiskPolicy::Instance() {
  static InstanceRiskPolicy instance;
  return instance;
}

const std::string& InstanceRiskPolicy::TableName() const {
  static const std::string kName = "instance_risk_policy";
  return kName;
}

const std::vector<std::string>& InstanceRiskPolicy::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& InstanceRiskPolicy::GetIndexSqls() const {
  static const std::vector<std::string> kSqls = {
    R"(CREATE INDEX IF NOT EXISTS idx_instance_risk_policy_engine ON instance_risk_policy (tenant_id, engine_id);)"
  };
  return kSqls;
}

}  // namespace qtrade::framework::dao
