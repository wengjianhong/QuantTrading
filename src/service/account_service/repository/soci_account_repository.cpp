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
  account_id TEXT NOT NULL,
  tenant_id TEXT NOT NULL,
  broker_id TEXT NOT NULL,
  connection_string TEXT NOT NULL,
  status TEXT NOT NULL,
  PRIMARY KEY (account_id)
);

CREATE TABLE IF NOT EXISTS account_credential (
  account_id TEXT NOT NULL,
  key_id TEXT NOT NULL,
  ciphertext TEXT NOT NULL,
  version INTEGER NOT NULL,
  PRIMARY KEY (account_id)
);

CREATE TABLE IF NOT EXISTS account_engine_binding (
  account_id TEXT NOT NULL,
  engine_id TEXT NOT NULL,
  PRIMARY KEY (account_id, engine_id)
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

}  // namespace

SociAccountRepository::SociAccountRepository(const DatabaseOptions& options) {
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

ErrorCode SociAccountRepository::RegisterAccount(const qtrade::account::v1::TradingAccount& account,
                                               const std::string& password) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (account.account_id().empty() || password.empty()) {
    return ErrorCode::kInternal;
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

  std::ostringstream upsert_account;
  upsert_account << "UPDATE trading_account SET tenant_id = '" << EscapeSqlLiteral(account.tenant_id())
                 << "', broker_id = '" << EscapeSqlLiteral(account.broker_id()) << "', connection_string = '"
                 << EscapeSqlLiteral(account.connection_string()) << "', status = '" << EscapeSqlLiteral(status)
                 << "' WHERE account_id = '" << EscapeSqlLiteral(account.account_id()) << "'";

  cpp_utils::database::ExecuteResult update_result;
  if (const auto rc = connection_->Execute(upsert_account.str(), &update_result);
      rc != cpp_utils::database::Error::kSuccess) {
    (void)tx.Rollback();
    return MapDbError(rc);
  }

  if (update_result.affected_rows == 0) {
    std::ostringstream insert_account;
    insert_account << "INSERT INTO trading_account(account_id, tenant_id, broker_id, connection_string, status) VALUES('"
                   << EscapeSqlLiteral(account.account_id()) << "', '" << EscapeSqlLiteral(account.tenant_id())
                   << "', '" << EscapeSqlLiteral(account.broker_id()) << "', '"
                   << EscapeSqlLiteral(account.connection_string()) << "', '" << EscapeSqlLiteral(status) << "')";
    if (const auto rc = connection_->Execute(insert_account.str()); rc != cpp_utils::database::Error::kSuccess) {
      (void)tx.Rollback();
      return MapDbError(rc);
    }
  }

  std::ostringstream upsert_credential;
  upsert_credential << "UPDATE account_credential SET key_id = '" << EscapeSqlLiteral(key_id) << "', ciphertext = '"
                    << EscapeSqlLiteral(ciphertext) << "', version = version + 1 WHERE account_id = '"
                    << EscapeSqlLiteral(account.account_id()) << "'";

  if (const auto rc = connection_->Execute(upsert_credential.str(), &update_result);
      rc != cpp_utils::database::Error::kSuccess) {
    (void)tx.Rollback();
    return MapDbError(rc);
  }

  if (update_result.affected_rows == 0) {
    std::ostringstream insert_credential;
    insert_credential << "INSERT INTO account_credential(account_id, key_id, ciphertext, version) VALUES('"
                      << EscapeSqlLiteral(account.account_id()) << "', '" << EscapeSqlLiteral(key_id) << "', '"
                      << EscapeSqlLiteral(ciphertext) << "', 1)";
    if (const auto rc = connection_->Execute(insert_credential.str()); rc != cpp_utils::database::Error::kSuccess) {
      (void)tx.Rollback();
      return MapDbError(rc);
    }
  }

  if (const auto rc = tx.Commit(); rc != cpp_utils::database::Error::kSuccess) {
    return MapDbError(rc);
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::RotateCredential(const std::string& account_id, const std::string& password) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (account_id.empty() || password.empty()) {
    return ErrorCode::kInternal;
  }

  std::string key_id;
  std::string ciphertext;
  if (!EncryptCredential(password, key_id, ciphertext)) {
    return ErrorCode::kInternal;
  }

  std::ostringstream sql;
  sql << "UPDATE account_credential SET key_id = '" << EscapeSqlLiteral(key_id) << "', ciphertext = '"
      << EscapeSqlLiteral(ciphertext) << "', version = version + 1 WHERE account_id = '"
      << EscapeSqlLiteral(account_id) << "'";

  cpp_utils::database::ExecuteResult result;
  if (const auto rc = connection_->Execute(sql.str(), &result); rc != cpp_utils::database::Error::kSuccess) {
    return MapDbError(rc);
  }
  if (result.affected_rows == 0) {
    return ErrorCode::kNotFound;
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::BindAccountToEngine(const std::string& account_id, const std::string& engine_id) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (account_id.empty() || engine_id.empty()) {
    return ErrorCode::kInternal;
  }

  std::ostringstream sql;
  sql << "INSERT INTO account_engine_binding(account_id, engine_id) VALUES('" << EscapeSqlLiteral(account_id)
      << "', '" << EscapeSqlLiteral(engine_id) << "')";

  if (const auto rc = connection_->Execute(sql.str()); rc != cpp_utils::database::Error::kSuccess) {
    return MapDbError(rc);
  }
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
    qtrade::account::v1::TradingAccount account;
    if (const auto cell = row->get_value("account_id")) {
      if (const auto v = cell->as_string()) {
        account.set_account_id(v.value());
      }
    }
    if (const auto cell = row->get_value("tenant_id")) {
      if (const auto v = cell->as_string()) {
        account.set_tenant_id(v.value());
      }
    }
    if (const auto cell = row->get_value("broker_id")) {
      if (const auto v = cell->as_string()) {
        account.set_broker_id(v.value());
      }
    }
    if (const auto cell = row->get_value("connection_string")) {
      if (const auto v = cell->as_string()) {
        account.set_connection_string(v.value());
      }
    }
    if (const auto cell = row->get_value("status")) {
      if (const auto v = cell->as_string()) {
        account.set_status(v.value());
      }
    }
    accounts.push_back(std::move(account));
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociAccountRepository::ResolveCredential(const std::string& engine_id,
                                                 const std::string& account_id,
                                                 qtrade::account::v1::ResolveCredentialResponse& response) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  if (engine_id.empty() || account_id.empty()) {
    return ErrorCode::kInternal;
  }

  std::ostringstream binding_sql;
  binding_sql << "SELECT 1 FROM account_engine_binding WHERE account_id = '" << EscapeSqlLiteral(account_id)
              << "' AND engine_id = '" << EscapeSqlLiteral(engine_id) << "'";

  auto [binding_err, binding_result] = connection_->Query(binding_sql.str());
  if (binding_err != cpp_utils::database::Error::kSuccess || binding_result == nullptr) {
    return MapDbError(binding_err);
  }
  if (!binding_result->Fetch().has_value()) {
    spdlog::warn("[SociAccountRepository] engine {} not authorized for account {}", engine_id, account_id);
    return ErrorCode::kNotFound;
  }

  std::ostringstream account_sql;
  account_sql << "SELECT broker_id, connection_string, status FROM trading_account WHERE account_id = '"
              << EscapeSqlLiteral(account_id) << "'";

  auto [account_err, account_result] = connection_->Query(account_sql.str());
  if (account_err != cpp_utils::database::Error::kSuccess || account_result == nullptr) {
    return MapDbError(account_err);
  }
  const auto account_row = account_result->Fetch();
  if (!account_row.has_value()) {
    return ErrorCode::kNotFound;
  }

  std::optional<std::string> broker_id;
  std::optional<std::string> connection_string;
  std::optional<std::string> status;
  if (const auto cell = account_row->get_value("broker_id")) {
    broker_id = cell->as_string();
  }
  if (const auto cell = account_row->get_value("connection_string")) {
    connection_string = cell->as_string();
  }
  if (const auto cell = account_row->get_value("status")) {
    status = cell->as_string();
  }
  if (!broker_id.has_value() || !connection_string.has_value()) {
    return ErrorCode::kSystemError;
  }
  if (status.has_value() && status.value() == "disabled") {
    return ErrorCode::kInternal;
  }

  std::ostringstream credential_sql;
  credential_sql << "SELECT key_id, ciphertext FROM account_credential WHERE account_id = '"
                 << EscapeSqlLiteral(account_id) << "'";

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

  std::string password;
  if (!DecryptCredential(key_id.value(), ciphertext.value(), password)) {
    return ErrorCode::kInternal;
  }

  response.set_account_id(account_id);
  response.set_broker_id(broker_id.value());
  response.set_connection_string(connection_string.value());
  response.set_password(password);
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::service
