/// @file      order_risk_policy.cpp
/// @brief     order_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/order_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

constexpr const char* kCreateTableSql = R"(
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

}  // namespace

OrderRiskPolicy& OrderRiskPolicy::Instance() {
  static OrderRiskPolicy instance;
  return instance;
}

const std::string& OrderRiskPolicy::TableName() const {
  static const std::string kName = "order_risk_policy";
  return kName;
}

const std::vector<std::string>& OrderRiskPolicy::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& OrderRiskPolicy::GetIndexSqls() const {
  static const std::vector<std::string> kEmpty;
  return kEmpty;
}

}  // namespace qtrade::framework::dao
