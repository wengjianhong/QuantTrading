/// @file      account_credential.cpp
/// @brief     account_credential 表 DAO 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/account_service/account_credential.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS account_credential (
  tenant_id TEXT NOT NULL COMMENT '租户 ID（与 trading_account 对齐）',
  account_id TEXT NOT NULL COMMENT '交易账户 ID（与 trading_account 对齐）',
  key_id TEXT NOT NULL COMMENT '加密密钥标识（解密选钥/轮换；与凭证内容版本无关）',
  credential_type INTEGER NOT NULL COMMENT '凭证类型（0=default, 1=password, 2=auth_code）',
  ciphertext TEXT NOT NULL COMMENT '凭证密文（可逆加密；勿存明文）',
  PRIMARY KEY (tenant_id, account_id, credential_type)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "account";

/// @brief 逻辑表名
const std::string kTableName = "account_credential";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {};

}  // namespace

const std::string& AccountCredential::DatabaseName() const {
  return kDatabaseName;
}

const std::string& AccountCredential::TableName() const {
  return kTableName;
}

const std::vector<std::string>& AccountCredential::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& AccountCredential::GetIndexSqls() const {
  return kIndexSqls;
}

/// @brief 将 AccountCredentialRecord 转为 KeyValues
KeyValues BuildAccountCredentialValues(const AccountCredentialRecord& record) {
  KeyValues values;
  AddTextValue(values, "tenant_id", record.tenant_id);
  AddTextValue(values, "account_id", record.account_id);
  if (record.credential_type.has_value()) {
    AddInt64Value(values, "credential_type", static_cast<std::int64_t>(record.credential_type.value()));
  }
  AddTextValue(values, "key_id", record.key_id);
  AddTextValue(values, "ciphertext", record.ciphertext);
  return values;
}

Result<std::int64_t> AccountCredential::Insert(cpputils::database::IConnection& connection,
                                               const std::vector<AccountCredentialRecord>& records) {
  if (records.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  // 1. 逐条转换并写入
  std::int64_t affected = 0;
  for (const auto& record : records) {
    const KeyValues values = BuildAccountCredentialValues(record);
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

Result<std::int64_t> AccountCredential::Delete(cpputils::database::IConnection& connection,
                                               const AccountCredentialRecord& where_conditions) {
  const KeyValues where_values = BuildAccountCredentialValues(where_conditions);
  if (where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return DeleteRows(connection, TableName(), where_values);
}

Result<std::int64_t> AccountCredential::BatchDelete(cpputils::database::IConnection&,
                                                    const std::vector<std::int64_t>&) {
  return Result<std::int64_t>{ErrorCode::kInternalError, "composite primary key"};
}

Result<std::int64_t> AccountCredential::Update(cpputils::database::IConnection& connection,
                                               const AccountCredentialRecord& record,
                                               const AccountCredentialRecord& where_conditions) {
  const KeyValues values = BuildAccountCredentialValues(record);
  const KeyValues where_values = BuildAccountCredentialValues(where_conditions);
  if (values.empty() || where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return UpdateRows(connection, TableName(), values, where_values);
}

Result<std::int64_t> AccountCredential::Count(cpputils::database::IConnection& connection,
                                              const AccountCredentialRecord& where_conditions) {
  return CountRows(connection, TableName(), BuildAccountCredentialValues(where_conditions));
}

Result<std::vector<AccountCredentialRecord>> AccountCredential::Select(
  cpputils::database::IConnection& connection, const AccountCredentialRecord& where_conditions) {
  // 1. 按条件查询并映射结果行
  auto query_result = SelectRows(connection, TableName(), BuildAccountCredentialValues(where_conditions));
  if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value()) {
    return Result<std::vector<AccountCredentialRecord>>{query_result.error_code, query_result.error_message};
  }

  std::vector<AccountCredentialRecord> rows;
  while (const auto row = query_result.data.value()->Fetch()) {
    rows.push_back(BuildAccountCredentialRecord(**row));
  }
  return Result<std::vector<AccountCredentialRecord>>{ErrorCode::kSuccess, "", std::move(rows)};
}

Result<std::int64_t> AccountCredential::Truncate(cpputils::database::IConnection& connection) {
  return TruncateRows(connection, TableName());
}

}  // namespace qtrade::framework::dao
