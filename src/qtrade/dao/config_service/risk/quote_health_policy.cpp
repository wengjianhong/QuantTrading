/// @file      quote_health_policy.cpp
/// @brief     quote_health_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/quote_health_policy.hpp"

namespace qtrade::framework::dao {
namespace {

constexpr const char* kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS quote_health_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  engine_id TEXT NOT NULL COMMENT '引擎实例 ID',
  instrument_id TEXT NOT NULL COMMENT '品种 ID；空串表示引擎缺省策略',
  version BIGINT NOT NULL COMMENT '策略配置版本',
  max_quote_latency_ms BIGINT NOT NULL COMMENT '行情延迟上限（毫秒）',
  max_stale_age_ms BIGINT NOT NULL COMMENT '行情时间戳陈旧上限（毫秒）',
  allow_sequence_gap BOOLEAN NOT NULL COMMENT '是否允许序号缺口继续交易',
  reject_on_failover BOOLEAN NOT NULL COMMENT '行情源切换期间是否拒绝新开仓',
  reject_new_open_on_unhealthy BOOLEAN NOT NULL COMMENT '不健康时是否拒绝受影响品种新开仓',
  block_open_on_limit_up_down BOOLEAN NOT NULL COMMENT '涨跌停状态是否禁止新开仓',
  enabled BOOLEAN NOT NULL COMMENT '是否启用行情健康校验',
  PRIMARY KEY (tenant_id, engine_id, instrument_id)
);
)";

}  // namespace

QuoteHealthPolicy& QuoteHealthPolicy::Instance() {
  static QuoteHealthPolicy instance;
  return instance;
}

const std::string& QuoteHealthPolicy::TableName() const {
  static const std::string kName = "quote_health_policy";
  return kName;
}

const std::vector<std::string>& QuoteHealthPolicy::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& QuoteHealthPolicy::GetIndexSqls() const {
  static const std::vector<std::string> kSqls = {
    R"(CREATE INDEX IF NOT EXISTS idx_quote_health_policy_engine ON quote_health_policy (tenant_id, engine_id);)"
  };
  return kSqls;
}

}  // namespace qtrade::framework::dao
