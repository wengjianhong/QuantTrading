/// @file      trading_account.cpp
/// @brief     trading_account 表 DAO 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/account_service/trading_account.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS trading_account (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  account_id TEXT NOT NULL COMMENT '交易账户 ID',
  broker_id TEXT NOT NULL COMMENT '券商 ID',
  connection_string TEXT NOT NULL COMMENT '交易通道连接串',
  status TEXT NOT NULL COMMENT '账户状态（如 active / disabled）',
  PRIMARY KEY (tenant_id, account_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "account";

/// @brief 逻辑表名
const std::string kTableName = "trading_account";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {};

}  // namespace

const std::string& TradingAccount::DatabaseName() const {
  return kDatabaseName;
}

const std::string& TradingAccount::TableName() const {
  return kTableName;
}

const std::vector<std::string>& TradingAccount::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& TradingAccount::GetIndexSqls() const {
  return kIndexSqls;
}

/// @brief 将 TradingAccountRecord 转为 KeyValues
KeyValues BuildTradingAccountValues(const TradingAccountRecord& record) {
  KeyValues values;
  AddTextValue(values, "tenant_id", record.tenant_id);
  AddTextValue(values, "account_id", record.account_id);
  AddTextValue(values, "broker_id", record.broker_id);
  AddTextValue(values, "connection_string", record.connection_string);
  AddTextValue(values, "status", record.status);
  return values;
}

Result<std::int64_t> TradingAccount::Insert(cpputils::database::IConnection& connection,
                                            const std::vector<TradingAccountRecord>& records) {
  if (records.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  // 1. 逐条转换并写入
  std::int64_t affected = 0;
  for (const auto& record : records) {
    const KeyValues values = BuildTradingAccountValues(record);
    if (values.empty()) {
      return Result<std::int64_t>{ErrorCode::kSystemError};
    }
    const auto result = InsertRow(connection, TableName(), values);
    if (result.error_code != ErrorCode::kSuccess) {
      return result;
    }
    affected += result.data.value_or(0);
  }
  return Result<std::int64_t>{ErrorCode::kSuccess, "", affected};
}

Result<std::int64_t> TradingAccount::Delete(cpputils::database::IConnection& connection,
                                            const TradingAccountRecord& where_conditions) {
  const KeyValues where_values = BuildTradingAccountValues(where_conditions);
  if (where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return DeleteRows(connection, TableName(), where_values);
}

Result<std::int64_t> TradingAccount::BatchDelete(cpputils::database::IConnection&, const std::vector<std::int64_t>&) {
  return Result<std::int64_t>{ErrorCode::kInternalError, "composite primary key"};
}

Result<std::int64_t> TradingAccount::Update(cpputils::database::IConnection& connection,
                                            const TradingAccountRecord& record,
                                            const TradingAccountRecord& where_conditions) {
  const KeyValues values = BuildTradingAccountValues(record);
  const KeyValues where_values = BuildTradingAccountValues(where_conditions);
  if (values.empty() || where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return UpdateRows(connection, TableName(), values, where_values);
}

Result<std::int64_t> TradingAccount::Count(cpputils::database::IConnection& connection,
                                           const TradingAccountRecord& where_conditions) {
  return CountRows(connection, TableName(), BuildTradingAccountValues(where_conditions));
}

Result<std::vector<TradingAccountRecord>> TradingAccount::Select(cpputils::database::IConnection& connection,
                                                                 const TradingAccountRecord& where_conditions) {
  // 1. 按条件查询并映射结果行
  auto query_result = SelectRows(connection, TableName(), BuildTradingAccountValues(where_conditions));
  if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value()) {
    return Result<std::vector<TradingAccountRecord>>{query_result.error_code};
  }

  std::vector<TradingAccountRecord> rows;
  while (const auto row = query_result.data.value()->Fetch()) {
    rows.push_back(BuildTradingAccountRecord(**row));
  }
  return Result<std::vector<TradingAccountRecord>>{ErrorCode::kSuccess, "", std::move(rows)};
}

Result<std::int64_t> TradingAccount::Truncate(cpputils::database::IConnection& connection) {
  return TruncateRows(connection, TableName());
}

}  // namespace qtrade::framework::dao
