/// @file      strategy_risk_policy.cpp
/// @brief     strategy_risk_policy 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/risk/strategy_risk_policy.hpp"

namespace qtrade::framework::dao {
namespace {

constexpr const char* kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS strategy_risk_policy (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '允许交易的账户 ID',
  engine_id TEXT NOT NULL COMMENT '引擎实例 ID',
  strategy_id TEXT NOT NULL COMMENT '策略 ID',
  version BIGINT NOT NULL COMMENT '限额配置版本',
  max_capital DOUBLE NOT NULL COMMENT '策略资金预算',
  max_position DOUBLE NOT NULL COMMENT '策略最大仓位（名义或数量口径由配置约定）',
  max_daily_loss DOUBLE NOT NULL COMMENT '策略单日损失上限',
  max_order_rate_per_sec BIGINT NOT NULL COMMENT '策略订单频率上限（笔/秒）',
  max_cancel_rate_per_sec BIGINT NOT NULL COMMENT '策略撤单频率上限（笔/秒）',
  allowed_instruments_json TEXT NOT NULL COMMENT '允许品种 JSON 数组；空表示不额外限制',
  enabled BOOLEAN NOT NULL COMMENT '是否启用策略级限额',
  PRIMARY KEY (tenant_id, account_id, engine_id, strategy_id)
);
)";

}  // namespace

StrategyRiskPolicy& StrategyRiskPolicy::Instance() {
  static StrategyRiskPolicy instance;
  return instance;
}

const std::string& StrategyRiskPolicy::TableName() const {
  static const std::string kName = "strategy_risk_policy";
  return kName;
}

const std::vector<std::string>& StrategyRiskPolicy::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& StrategyRiskPolicy::GetIndexSqls() const {
  static const std::vector<std::string> kSqls = {
    R"(CREATE INDEX IF NOT EXISTS idx_strategy_risk_policy_strategy ON strategy_risk_policy (tenant_id, strategy_id);)"
  };
  return kSqls;
}

}  // namespace qtrade::framework::dao
