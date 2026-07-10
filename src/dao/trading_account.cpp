/// @file      trading_account.cpp
/// @brief     trading_account 表 DAO 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "dao/trading_account.hpp"

#include "common/dao/sql_utils.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::framework::dao {
namespace {

TradingAccount* g_mock_instance = nullptr;  ///< 测试 Mock 实例指针

constexpr const char* kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS trading_account (
  tenant_id TEXT NOT NULL,
  account_id TEXT NOT NULL,
  broker_id TEXT NOT NULL,
  connection_string TEXT NOT NULL,
  status TEXT NOT NULL,
  PRIMARY KEY (tenant_id, account_id)
);
)";

}  // namespace

TradingAccount& TradingAccount::Instance() {
  if (g_mock_instance != nullptr) {
    return *g_mock_instance;
  }
  static TradingAccount instance;
  return instance;
}

void TradingAccount::SetMockInstance(TradingAccount* mock_instance) { g_mock_instance = mock_instance; }

void TradingAccount::ClearMockInstance() { g_mock_instance = nullptr; }

const std::string& TradingAccount::TableName() const {
  static const std::string kName = "trading_account";
  return kName;
}

const std::vector<std::string>& TradingAccount::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& TradingAccount::GetIndexSqls() const {
  static const std::vector<std::string> kEmpty;
  return kEmpty;
}

KeyValues BuildTradingAccountValues(const TradingAccountRecord& record) {
  KeyValues values;
  AddTextValue(values, "tenant_id", record.tenant_id);
  AddTextValue(values, "account_id", record.account_id);
  AddTextValue(values, "broker_id", record.broker_id);
  AddTextValue(values, "connection_string", record.connection_string);
  AddTextValue(values, "status", record.status);
  return values;
}

Result<std::int64_t> TradingAccount::Insert(const std::vector<TradingAccountRecord>& records) {
  if (records.empty()) {
    return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
  }

  std::int64_t affected = 0;
  for (const auto& record : records) {
    const KeyValues values = BuildTradingAccountValues(record);
    if (values.empty()) {
      return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
    }
    const auto result = InsertRow(TableName(), values);
    if (result.error_code != ErrorCode::kSuccess) {
      return result;
    }
    affected += result.data.value_or(0);
  }
  return Result<std::int64_t>{.data = affected};
}

Result<std::int64_t> TradingAccount::Delete(const TradingAccountRecord& where_conditions) {
  const KeyValues where_values = BuildTradingAccountValues(where_conditions);
  if (where_values.empty()) {
    return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
  }
  return DeleteRows(TableName(), where_values);
}

Result<std::int64_t> TradingAccount::BatchDelete(const std::vector<std::int64_t>&) {
  return Result<std::int64_t>{.error_code = ErrorCode::kInternal, .error_message = "composite primary key"};
}

Result<std::int64_t> TradingAccount::Update(const TradingAccountRecord& record, const TradingAccountRecord& where_conditions) {
  const KeyValues values = BuildTradingAccountValues(record);
  const KeyValues where_values = BuildTradingAccountValues(where_conditions);
  if (values.empty() || where_values.empty()) {
    return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
  }
  return UpdateRows(TableName(), values, where_values);
}

Result<std::int64_t> TradingAccount::Count(const TradingAccountRecord& where_conditions) {
  return CountRows(TableName(), BuildTradingAccountValues(where_conditions));
}

Result<std::vector<TradingAccountRecord>> TradingAccount::Select(const TradingAccountRecord& where_conditions) {
  auto query_result = SelectRows(TableName(), BuildTradingAccountValues(where_conditions));
  if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value()) {
    return Result<std::vector<TradingAccountRecord>>{.error_code = query_result.error_code};
  }

  std::vector<TradingAccountRecord> rows;
  while (const auto row = query_result.data.value()->Fetch()) {
    rows.push_back(BuildTradingAccountRecord(**row));
  }
  return Result<std::vector<TradingAccountRecord>>{.data = std::move(rows)};
}

Result<std::int64_t> TradingAccount::Truncate() {
  return TruncateRows(TableName());
}

}  // namespace qtrade::framework::dao
