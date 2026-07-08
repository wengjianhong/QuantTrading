/// @file      soci_account_repository.cpp
/// @brief     SociAccountRepository 实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "service/account_service/repository/soci_account_repository.hpp"

#include "service/account_service/repository/credential_codec.hpp"

#include <cpputils/database/database.hpp>

#include <spdlog/spdlog.h>

#include <sstream>
#include <utility>

namespace qtrade::service {
namespace {

constexpr const char* kEnsureSchemaSql = R"(
CREATE TABLE IF NOT EXISTS trading_account (
  tenant_id TEXT NOT NULL,
  account_id TEXT NOT NULL,
  broker_id TEXT NOT NULL,
  connection_string TEXT NOT NULL,
  status TEXT NOT NULL,
  PRIMARY KEY (tenant_id, account_id)
);

CREATE TABLE IF NOT EXISTS account_credential (
  tenant_id TEXT NOT NULL,
  account_id TEXT NOT NULL,
  key_id TEXT NOT NULL,
  ciphertext TEXT NOT NULL,
  version INTEGER NOT NULL,
  PRIMARY KEY (tenant_id, account_id)
);
)";

std::string EscapeSqlLiteral(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char ch : value) {
    if (ch == '\'') {
      escaped += "''";
    } else {
      escaped += ch;
    }
  }
  return escaped;
}

std::string AccountKeyWhereClause(const std::string& tenant_id, const std::string& account_id) {
  std::ostringstream sql;
  sql << "tenant_id = '" << EscapeSqlLiteral(tenant_id) << "' AND account_id = '"
      << EscapeSqlLiteral(account_id) << "'";
  return sql.str();
}

ErrorCode MapDbError(cpp_utils::database::Error error) {
  switch (error) {
    case cpp_utils::database::Error::kSuccess:
      return ErrorCode::kSuccess;
    case cpp_utils::database::Error::kNotFound:
      return ErrorCode::kNotFound;
    case cpp_utils::database::Error::kInvalidArgument:
      return ErrorCode::kInternal;
    default:
      return ErrorCode::kSystemError;
  }
}

qtrade::account::v1::TradingAccount ParseTradingAccountRow(const cpp_utils::database::Row& row) {
  qtrade::account::v1::TradingAccount account;
  if (const auto cell = row.get_value("account_id")) {
    if (const auto v = cell->as_string()) {
      account.set_account_id(v.value());
    }
  }
  if (const auto cell = row.get_value("tenant_id")) {
    if (const auto v = cell->as_string()) {
      account.set_tenant_id(v.value());
    }
  }
  if (const auto cell = row.get_value("broker_id")) {
    if (const auto v = cell->as_string()) {
      account.set_broker_id(v.value());
    }
  }
  if (const auto cell = row.get_value("connection_string")) {
    if (const auto v = cell->as_string()) {
      account.set_connection_string(v.value());
    }
  }
  if (const auto cell = row.get_value("status")) {
    if (const auto v = cell->as_string()) {
      account.set_status(v.value());
    }
  }
  return account;
}

}  // namespace

SociAccountRepository::SociAccountRepository(const qtrade::common::DatabaseOptions& options) {
  if (options.pool.has_value()) {
    pool_ = cpp_utils::database::CreateConnectionPool();
    if (const auto rc = pool_->Open(*options.pool); rc != cpp_utils::database::Error::kSuccess) {
      spdlog::error("[SociAccountRepository] open pool failed");
      pool_.reset();
      return;
    }
    connection_ = pool_->Acquire();
    if (!connection_) {
      spdlog::error("[SociAccountRepository] acquire connection failed");
    }
    return;
  }

  auto owned = std::make_unique<cpp_utils::database::Connection>(options.connection);
  if (const auto rc = owned->Connect(); rc != cpp_utils::database::Error::kSuccess) {
    spdlog::error("[SociAccountRepository] connect failed: {}", owned->LastError());
  }
  connection_ = std::move(owned);
}

SociAccountRepository::~SociAccountRepository() {
  connection_.reset();
  if (pool_) {
    pool_->Close();
    pool_.reset();
  }
}

bool SociAccountRepository::IsReady() const { return connection_ != nullptr && connection_->IsConnected(); }

ErrorCode SociAccountRepository::EnsureSchema() {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  const auto rc = connection_->Execute(kEnsureSchemaSql);
  if (rc != cpp_utils::database::Error::kSuccess) {
    spdlog::error("[SociAccountRepository] ensure schema failed: {}", connection_->LastError());
  }
  return MapDbError(rc);
}

ErrorCode SociAccountRepository::AddAccount(const qtrade::account::v1::TradingAccount& account) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (account.tenant_id().empty() || account.account_id().empty() || account.password().empty()) {
    return ErrorCode::kInternal;
  }

  const std::string& password = account.password();

  std::ostringstream exists_sql;
  exists_sql << "SELECT 1 FROM trading_account WHERE " << AccountKeyWhereClause(account.tenant_id(), account.account_id());
  auto [exists_err, exists_result] = connection_->Query(exists_sql.str());
  if (exists_err != cpp_utils::database::Error::kSuccess || exists_result == nullptr) {
    return MapDbError(exists_err);
  }
  if (exists_result->Fetch().has_value()) {
    return ErrorCode::kSystemError;
  }

  std::string key_id;
  std::string ciphertext;
  if (!EncryptCredential(password, key_id, ciphertext)) {
    return ErrorCode::kInternal;
  }

  auto [begin_rc, tx] = connection_->BeginTransaction();
  if (begin_rc != cpp_utils::database::Error::kSuccess) {
    return MapDbError(begin_rc);
  }

  const std::string status = account.status().empty() ? "active" : account.status();

  std::ostringstream insert_account;
  insert_account << "INSERT INTO trading_account(tenant_id, account_id, broker_id, connection_string, status) VALUES('"
                 << EscapeSqlLiteral(account.tenant_id()) << "', '" << EscapeSqlLiteral(account.account_id()) << "', '"
                 << EscapeSqlLiteral(account.broker_id()) << "', '" << EscapeSqlLiteral(account.connection_string())
                 << "', '" << EscapeSqlLiteral(status) << "')";
  if (const auto rc = connection_->Execute(insert_account.str()); rc != cpp_utils::database::Error::kSuccess) {
    (void)tx.Rollback();
    return MapDbError(rc);
  }

  std::ostringstream insert_credential;
  insert_credential << "INSERT INTO account_credential(tenant_id, account_id, key_id, ciphertext, version) VALUES('"
                    << EscapeSqlLiteral(account.tenant_id()) << "', '" << EscapeSqlLiteral(account.account_id())
                    << "', '" << EscapeSqlLiteral(key_id) << "', '" << EscapeSqlLiteral(ciphertext) << "', 1)";
  if (const auto rc = connection_->Execute(insert_credential.str()); rc != cpp_utils::database::Error::kSuccess) {
    (void)tx.Rollback();
    return MapDbError(rc);
  }

  if (const auto rc = tx.Commit(); rc != cpp_utils::database::Error::kSuccess) {
    return MapDbError(rc);
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

  std::ostringstream sql;
  sql << "SELECT account_id, tenant_id, broker_id, connection_string, status FROM trading_account WHERE "
      << AccountKeyWhereClause(tenant_id, account_id);

  auto [query_err, result] = connection_->Query(sql.str());
  if (query_err != cpp_utils::database::Error::kSuccess || result == nullptr) {
    return MapDbError(query_err);
  }
  const auto row = result->Fetch();
  if (!row.has_value()) {
    return ErrorCode::kNotFound;
  }

  account = ParseTradingAccountRow(*row);
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::ListAccounts(const std::string& tenant_id,
                                              std::vector<qtrade::account::v1::TradingAccount>& accounts) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  accounts.clear();

  std::ostringstream sql;
  sql << "SELECT account_id, tenant_id, broker_id, connection_string, status FROM trading_account";
  if (!tenant_id.empty()) {
    sql << " WHERE tenant_id = '" << EscapeSqlLiteral(tenant_id) << "'";
  }

  auto [query_err, result] = connection_->Query(sql.str());
  if (query_err != cpp_utils::database::Error::kSuccess || result == nullptr) {
    return MapDbError(query_err);
  }

  while (const auto row = result->Fetch()) {
    accounts.push_back(ParseTradingAccountRow(*row));
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

  auto [begin_rc, tx] = connection_->BeginTransaction();
  if (begin_rc != cpp_utils::database::Error::kSuccess) {
    return MapDbError(begin_rc);
  }

  const std::string status = account.status().empty() ? "active" : account.status();
  std::ostringstream update_account;
  update_account << "UPDATE trading_account SET broker_id = '" << EscapeSqlLiteral(account.broker_id())
                 << "', connection_string = '" << EscapeSqlLiteral(account.connection_string()) << "', status = '"
                 << EscapeSqlLiteral(status) << "' WHERE " << AccountKeyWhereClause(account.tenant_id(), account.account_id());

  cpp_utils::database::ExecuteResult update_result;
  if (const auto rc = connection_->Execute(update_account.str(), &update_result);
      rc != cpp_utils::database::Error::kSuccess) {
    (void)tx.Rollback();
    return MapDbError(rc);
  }
  if (update_result.affected_rows == 0) {
    (void)tx.Rollback();
    return ErrorCode::kNotFound;
  }

  if (update_password) {
    const std::string& password = account.password();
    std::string key_id;
    std::string ciphertext;
    if (!EncryptCredential(password, key_id, ciphertext)) {
      (void)tx.Rollback();
      return ErrorCode::kInternal;
    }

    std::ostringstream update_credential;
    update_credential << "UPDATE account_credential SET key_id = '" << EscapeSqlLiteral(key_id) << "', ciphertext = '"
                      << EscapeSqlLiteral(ciphertext) << "', version = version + 1 WHERE "
                      << AccountKeyWhereClause(account.tenant_id(), account.account_id());
    if (const auto rc = connection_->Execute(update_credential.str(), &update_result);
        rc != cpp_utils::database::Error::kSuccess) {
      (void)tx.Rollback();
      return MapDbError(rc);
    }
    if (update_result.affected_rows == 0) {
      (void)tx.Rollback();
      return ErrorCode::kNotFound;
    }
  }

  if (const auto rc = tx.Commit(); rc != cpp_utils::database::Error::kSuccess) {
    return MapDbError(rc);
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

  std::ostringstream credential_sql;
  credential_sql << "SELECT key_id, ciphertext FROM account_credential WHERE "
                 << AccountKeyWhereClause(tenant_id, account_id);

  auto [cred_err, cred_result] = connection_->Query(credential_sql.str());
  if (cred_err != cpp_utils::database::Error::kSuccess || cred_result == nullptr) {
    return MapDbError(cred_err);
  }
  const auto cred_row = cred_result->Fetch();
  if (!cred_row.has_value()) {
    return ErrorCode::kNotFound;
  }

  std::optional<std::string> key_id;
  std::optional<std::string> ciphertext;
  if (const auto cell = cred_row->get_value("key_id")) {
    key_id = cell->as_string();
  }
  if (const auto cell = cred_row->get_value("ciphertext")) {
    ciphertext = cell->as_string();
  }
  if (!key_id.has_value() || !ciphertext.has_value()) {
    return ErrorCode::kSystemError;
  }

  std::string plain_password;
  if (!DecryptCredential(key_id.value(), ciphertext.value(), plain_password)) {
    return ErrorCode::kInternal;
  }

  spdlog::info("[SociAccountRepository] credential fetched for tenant={} engine={} account={}", tenant_id, engine_id,
               account_id);
  account.set_password(plain_password);
  *response.mutable_account() = std::move(account);
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::service
