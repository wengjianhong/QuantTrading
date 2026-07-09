/// @file      soci_account_repository.cpp
/// @brief     SociAccountRepository 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "service/account_service/repository/soci_account_repository.hpp"

#include "service/account_service/repository/credential_codec.hpp"

#include "common/dao/ddl_utils.hpp"
#include "common/dao/dml_utils.hpp"
#include "dao/account_credential.hpp"
#include "dao/trading_account.hpp"

#include <cpputils/database/database.hpp>

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::service {
namespace {

using qtrade::framework::dao::AccountCredentialRecord;
using qtrade::framework::dao::TradingAccountRecord;

/// @brief 将 proto 账户转为 DAO 记录
/// @param account proto 账户
/// @return TradingAccountRecord
TradingAccountRecord ToRecord(const qtrade::account::v1::TradingAccount& account) {
  TradingAccountRecord record;
  record.tenant_id = account.tenant_id();
  record.account_id = account.account_id();
  record.broker_id = account.broker_id();
  record.connection_string = account.connection_string();
  record.status = account.status().empty() ? "active" : account.status();
  return record;
}

/// @brief 将 DAO 记录转为 proto 账户
/// @param record DAO 记录
/// @param account 输出 proto 账户
void ToProto(const TradingAccountRecord& record, qtrade::account::v1::TradingAccount& account) {
  account.set_tenant_id(record.tenant_id.value_or(""));
  account.set_account_id(record.account_id.value_or(""));
  account.set_broker_id(record.broker_id.value_or(""));
  account.set_connection_string(record.connection_string.value_or(""));
  account.set_status(record.status.value_or(""));
}

}  // namespace

SociAccountRepository::SociAccountRepository(const qtrade::common::DatabaseOptions& options) : connection_(options) {
  if (IsReady()) {
    qtrade::framework::dao::SetConnection(connection_.Connection());
  }
}

SociAccountRepository::~SociAccountRepository() = default;

bool SociAccountRepository::IsReady() const { return connection_.IsReady(); }

ErrorCode SociAccountRepository::EnsureSchema() {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  const auto& trading_schema = qtrade::framework::dao::TradingAccount::Instance();
  const auto& credential_schema = qtrade::framework::dao::AccountCredential::Instance();
  if (const auto rc = qtrade::framework::dao::EnsureTableSchema(connection_.Connection(), trading_schema);
      rc != ErrorCode::kSuccess) {
    spdlog::error("[SociAccountRepository] ensure trading_account schema failed");
    return rc;
  }
  if (const auto rc = qtrade::framework::dao::EnsureTableSchema(connection_.Connection(), credential_schema);
      rc != ErrorCode::kSuccess) {
    spdlog::error("[SociAccountRepository] ensure account_credential schema failed");
    return rc;
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::AddAccount(const qtrade::account::v1::TradingAccount& account) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (account.tenant_id().empty() || account.account_id().empty() || account.password().empty()) {
    return ErrorCode::kInternal;
  }

  auto& trading_dao = qtrade::framework::dao::TradingAccount::Instance();
  auto& credential_dao = qtrade::framework::dao::AccountCredential::Instance();

  TradingAccountRecord key;
  key.tenant_id = account.tenant_id();
  key.account_id = account.account_id();
  const auto exists = trading_dao.Count(key);
  if (exists.error_code != ErrorCode::kSuccess) {
    return exists.error_code;
  }
  if (exists.data.has_value() && exists.data.value() > 0) {
    return ErrorCode::kSystemError;
  }

  std::string key_id;
  std::string ciphertext;
  if (!EncryptCredential(account.password(), key_id, ciphertext)) {
    return ErrorCode::kInternal;
  }

  auto* connection = connection_.Connection();
  auto [begin_rc, tx] = connection->BeginTransaction();
  if (begin_rc != cpp_utils::database::Error::kSuccess) {
    return ErrorCode::kSystemError;
  }

  // 1. 插入 trading_account 与 account_credential
  if (const auto insert_account = trading_dao.Insert({ToRecord(account)});
      insert_account.error_code != ErrorCode::kSuccess) {
    (void)tx.Rollback();
    return insert_account.error_code;
  }

  AccountCredentialRecord credential_row;
  credential_row.tenant_id = account.tenant_id();
  credential_row.account_id = account.account_id();
  credential_row.key_id = key_id;
  credential_row.ciphertext = ciphertext;
  credential_row.version = 1;
  if (const auto insert_credential = credential_dao.Insert({credential_row});
      insert_credential.error_code != ErrorCode::kSuccess) {
    (void)tx.Rollback();
    return insert_credential.error_code;
  }

  if (const auto rc = tx.Commit(); rc != cpp_utils::database::Error::kSuccess) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::GetAccount(const std::string& tenant_id,
                                            const std::string& account_id,
                                            qtrade::account::v1::TradingAccount& account) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (tenant_id.empty() || account_id.empty()) {
    return ErrorCode::kInternal;
  }

  TradingAccountRecord where;
  where.tenant_id = tenant_id;
  where.account_id = account_id;
  const auto result = qtrade::framework::dao::TradingAccount::Instance().Select(where);
  if (result.error_code != ErrorCode::kSuccess) {
    return result.error_code;
  }
  if (!result.data.has_value() || result.data->empty()) {
    return ErrorCode::kNotFound;
  }

  ToProto(result.data->front(), account);
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::ListAccounts(const std::string& tenant_id,
                                              std::vector<qtrade::account::v1::TradingAccount>& accounts) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  accounts.clear();

  TradingAccountRecord where;
  if (!tenant_id.empty()) {
    where.tenant_id = tenant_id;
  }

  const auto result = qtrade::framework::dao::TradingAccount::Instance().Select(where);
  if (result.error_code != ErrorCode::kSuccess || !result.data.has_value()) {
    return result.error_code;
  }

  accounts.reserve(result.data->size());
  for (const auto& row : *result.data) {
    qtrade::account::v1::TradingAccount account;
    ToProto(row, account);
    accounts.push_back(std::move(account));
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::UpdateAccount(const qtrade::account::v1::TradingAccount& account) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (account.tenant_id().empty() || account.account_id().empty()) {
    return ErrorCode::kInternal;
  }

  const bool update_password = !account.password().empty();

  auto& trading_dao = qtrade::framework::dao::TradingAccount::Instance();
  auto& credential_dao = qtrade::framework::dao::AccountCredential::Instance();

  auto* connection = connection_.Connection();
  auto [begin_rc, tx] = connection->BeginTransaction();
  if (begin_rc != cpp_utils::database::Error::kSuccess) {
    return ErrorCode::kSystemError;
  }

  TradingAccountRecord where;
  where.tenant_id = account.tenant_id();
  where.account_id = account.account_id();
  const auto update_result = trading_dao.Update(ToRecord(account), where);
  if (update_result.error_code != ErrorCode::kSuccess) {
    (void)tx.Rollback();
    return update_result.error_code;
  }
  if (!update_result.data.has_value() || update_result.data.value() == 0) {
    (void)tx.Rollback();
    return ErrorCode::kNotFound;
  }

  if (update_password) {
    // 2. 密码变更时更新凭证并递增 version
    std::string key_id;
    std::string ciphertext;
    if (!EncryptCredential(account.password(), key_id, ciphertext)) {
      (void)tx.Rollback();
      return ErrorCode::kInternal;
    }

    AccountCredentialRecord credential_key;
    credential_key.tenant_id = account.tenant_id();
    credential_key.account_id = account.account_id();
    const auto existing = credential_dao.Select(credential_key);
    if (existing.error_code != ErrorCode::kSuccess || !existing.data.has_value() || existing.data->empty()) {
      (void)tx.Rollback();
      return ErrorCode::kNotFound;
    }

    AccountCredentialRecord credential_row;
    credential_row.tenant_id = account.tenant_id();
    credential_row.account_id = account.account_id();
    credential_row.key_id = key_id;
    credential_row.ciphertext = ciphertext;
    credential_row.version = existing.data->front().version.value_or(0) + 1;
    const auto update_credential = credential_dao.Update(credential_row, credential_key);
    if (update_credential.error_code != ErrorCode::kSuccess) {
      (void)tx.Rollback();
      return update_credential.error_code;
    }
    if (!update_credential.data.has_value() || update_credential.data.value() == 0) {
      (void)tx.Rollback();
      return ErrorCode::kNotFound;
    }
  }

  if (const auto rc = tx.Commit(); rc != cpp_utils::database::Error::kSuccess) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::GetCredential(const std::string& tenant_id,
                                               const std::string& engine_id,
                                               const std::string& account_id,
                                               qtrade::account::v1::GetCredentialResponse& response) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (tenant_id.empty() || engine_id.empty() || account_id.empty()) {
    return ErrorCode::kInternal;
  }

  qtrade::account::v1::TradingAccount account;
  const auto account_rc = GetAccount(tenant_id, account_id, account);
  if (account_rc != ErrorCode::kSuccess) {
    return account_rc;
  }
  if (account.status() == "disabled") {
    return ErrorCode::kInternal;
  }

  AccountCredentialRecord where;
  where.tenant_id = tenant_id;
  where.account_id = account_id;
  const auto cred_result = qtrade::framework::dao::AccountCredential::Instance().Select(where);
  if (cred_result.error_code != ErrorCode::kSuccess) {
    return cred_result.error_code;
  }
  if (!cred_result.data.has_value() || cred_result.data->empty()) {
    return ErrorCode::kNotFound;
  }

  const auto& cred_row = cred_result.data->front();
  if (!cred_row.key_id.has_value() || !cred_row.ciphertext.has_value()) {
    return ErrorCode::kInternal;
  }

  std::string plain_password;
  if (!DecryptCredential(cred_row.key_id.value(), cred_row.ciphertext.value(), plain_password)) {
    return ErrorCode::kInternal;
  }

  spdlog::info(
    "[SociAccountRepository] credential fetched for tenant={} engine={} account={}", tenant_id, engine_id, account_id);
  account.set_password(plain_password);
  *response.mutable_account() = std::move(account);
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::service
