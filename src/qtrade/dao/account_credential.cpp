/// @file      account_credential.cpp
/// @brief     account_credential 表 DAO 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/account_credential.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::framework::dao {
namespace {

AccountCredential* g_mock_instance = nullptr;  ///< 测试 Mock 实例指针

constexpr const char* kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS account_credential (
  tenant_id TEXT NOT NULL COMMENT '租户 ID（与 trading_account 对齐）',
  account_id TEXT NOT NULL COMMENT '交易账户 ID（与 trading_account 对齐）',
  key_id TEXT NOT NULL COMMENT '加密密钥标识（解密选钥/轮换；与凭证内容版本无关）',
  credential_type INTEGER NOT NULL COMMENT '凭证类型（0=default, 1=password, 2=auth_code）',
  ciphertext TEXT NOT NULL COMMENT '凭证密文（可逆加密；勿存明文）',
  PRIMARY KEY (tenant_id, account_id, credential_type)
);
)";

}  // namespace

AccountCredential& AccountCredential::Instance() {
  if (g_mock_instance != nullptr) {
    return *g_mock_instance;
  }
  static AccountCredential instance;
  return instance;
}

void AccountCredential::SetMockInstance(AccountCredential* mock_instance) {
  g_mock_instance = mock_instance;
}

void AccountCredential::ClearMockInstance() {
  g_mock_instance = nullptr;
}

const std::string& AccountCredential::TableName() const {
  static const std::string kName = "account_credential";
  return kName;
}

const std::vector<std::string>& AccountCredential::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& AccountCredential::GetIndexSqls() const {
  static const std::vector<std::string> kEmpty;
  return kEmpty;
}

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

Result<std::int64_t> AccountCredential::Insert(const std::vector<AccountCredentialRecord>& records) {
  if (records.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  std::int64_t affected = 0;
  for (const auto& record : records) {
    const KeyValues values = BuildAccountCredentialValues(record);
    if (values.empty()) {
      return Result<std::int64_t>{ErrorCode::kSystemError};
    }
    const auto result = InsertRow(TableName(), values);
    if (result.error_code != ErrorCode::kSuccess) {
      return result;
    }
    affected += result.data.value_or(0);
  }
  return Result<std::int64_t>{ErrorCode::kSuccess, "", affected};
}

Result<std::int64_t> AccountCredential::Delete(const AccountCredentialRecord& where_conditions) {
  const KeyValues where_values = BuildAccountCredentialValues(where_conditions);
  if (where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return DeleteRows(TableName(), where_values);
}

Result<std::int64_t> AccountCredential::BatchDelete(const std::vector<std::int64_t>&) {
  return Result<std::int64_t>{ErrorCode::kInternal, "composite primary key"};
}

Result<std::int64_t> AccountCredential::Update(const AccountCredentialRecord& record,
                                               const AccountCredentialRecord& where_conditions) {
  const KeyValues values = BuildAccountCredentialValues(record);
  const KeyValues where_values = BuildAccountCredentialValues(where_conditions);
  if (values.empty() || where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return UpdateRows(TableName(), values, where_values);
}

Result<std::int64_t> AccountCredential::Count(const AccountCredentialRecord& where_conditions) {
  return CountRows(TableName(), BuildAccountCredentialValues(where_conditions));
}

Result<std::vector<AccountCredentialRecord>> AccountCredential::Select(
  const AccountCredentialRecord& where_conditions) {
  auto query_result = SelectRows(TableName(), BuildAccountCredentialValues(where_conditions));
  if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value()) {
    return Result<std::vector<AccountCredentialRecord>>{query_result.error_code, query_result.error_message};
  }

  std::vector<AccountCredentialRecord> rows;
  while (const auto row = query_result.data.value()->Fetch()) {
    rows.push_back(BuildAccountCredentialRecord(**row));
  }
  return Result<std::vector<AccountCredentialRecord>>{ErrorCode::kSuccess, "", std::move(rows)};
}

Result<std::int64_t> AccountCredential::Truncate() {
  return TruncateRows(TableName());
}

}  // namespace qtrade::framework::dao
