/// @file      order_reservation.cpp
/// @brief     order_reservation 表 DAO 实现（DDL）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/account_risk_service/order_reservation.hpp"

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS order_reservation (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '交易账户 ID',
  order_id TEXT NOT NULL COMMENT '全局订单 ID',
  reservation_id TEXT NOT NULL COMMENT '预占 ID',
  engine_id TEXT NOT NULL COMMENT '发起预占的引擎实例 ID',
  strategy_id TEXT NOT NULL COMMENT '策略 ID；可为空',
  instrument_id TEXT NOT NULL COMMENT '合约/品种 ID',
  side TEXT NOT NULL COMMENT '买卖方向',
  reserved_notional DOUBLE NOT NULL COMMENT '预占名义金额',
  reserved_margin DOUBLE NOT NULL COMMENT '预占保证金',
  quantity BIGINT NOT NULL COMMENT '预占数量',
  status TEXT NOT NULL COMMENT '预占状态：reserved / released / settled / expired',
  expires_at_unix_ms BIGINT NOT NULL COMMENT '预占过期时间（Unix 毫秒）',
  created_at_unix_ms BIGINT NOT NULL COMMENT '创建时间（Unix 毫秒）',
  updated_at_unix_ms BIGINT NOT NULL COMMENT '最近更新时间（Unix 毫秒）',
  PRIMARY KEY (tenant_id, account_id, order_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "account_risk";

/// @brief 逻辑表名
const std::string kTableName = "order_reservation";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {
  R"(CREATE INDEX IF NOT EXISTS idx_order_reservation_status ON order_reservation (tenant_id, account_id, status);)",
  R"(CREATE INDEX IF NOT EXISTS idx_order_reservation_expires ON order_reservation (expires_at_unix_ms);)"};

}  // namespace

const std::string& OrderReservation::DatabaseName() const {
  return kDatabaseName;
}

const std::string& OrderReservation::TableName() const {
  return kTableName;
}

const std::vector<std::string>& OrderReservation::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& OrderReservation::GetIndexSqls() const {
  return kIndexSqls;
}

}  // namespace qtrade::framework::dao
