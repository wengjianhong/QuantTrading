/// @file      add_account_handler.cpp
/// @brief     AddAccount：写入 trading_account 与 account_credential
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/add_account_handler.hpp"

#include "qtrade/dao/account_service/account_credential.hpp"
#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/dao/dao_define.hpp"
#include "qtrade/service/account_service/logic/credential_codec.hpp"
#include "qtrade/service/account_service/logic/trading_account_converter.hpp"

namespace qtrade::service {

namespace {

[[nodiscard]] bool OptionalStringEmpty(const std::optional<std::string>& value) {
  return !value.has_value() || value->empty();
}

}  // namespace

Result<AddAccountServerData> AddAccountHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::AddAccountRequest* request) {
  (void)context;
  if (!request->has_account()) {
    return Result<AddAccountServerData>{ErrorCode::kInternalError, "account is missing"};
  }

  AddAccountServerData data;
  data.account = ToTradingAccountRecord(request->account());
  data.password = request->account().password();
  return {ErrorCode::kSuccess, "success", std::move(data)};
}

Result<void> AddAccountHandler::ValidateParams(AddAccountServerData& server_data) {
  if (OptionalStringEmpty(server_data.account.tenant_id) || OptionalStringEmpty(server_data.account.account_id) ||
      server_data.password.empty()) {
    return Result<void>{ErrorCode::kInternalError, "tenant_id, account_id and password are required"};
  }
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> AddAccountHandler::CheckPreconditions(AddAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> AddAccountHandler::ExecuteBusiness(AddAccountServerData& server_data) {
  auto connection = pool_manager_.Acquire(qtrade::framework::dao::kAccountDatabaseName);
  if (connection == nullptr) {
    return Result<void>{ErrorCode::kSystemError, "database connection pool is unavailable"};
  }
  if (!connection->BeginTransaction()) {
    return Result<void>{ErrorCode::kSystemError, "begin database transaction failed"};
  }

  auto& trading_dao = dao_manager_.Get<qtrade::framework::dao::TradingAccount>();
  auto& credential_dao = dao_manager_.Get<qtrade::framework::dao::AccountCredential>();

  // 1. 查重
  qtrade::framework::dao::TradingAccountRecord key;
  key.tenant_id = server_data.account.tenant_id;
  key.account_id = server_data.account.account_id;
  const auto exists = trading_dao.Count(*connection, key);
  if (exists.error_code != ErrorCode::kSuccess) {
    (void)connection->RollbackTransaction();
    return Result<void>{exists.error_code, exists.error_message};
  }
  if (exists.data.has_value() && exists.data.value() > 0) {
    (void)connection->RollbackTransaction();
    return Result<void>{ErrorCode::kSystemError, "account already exists"};
  }

  // 2. 加密明文密码
  std::string key_id;
  std::string ciphertext;
  if (!EncryptCredential(server_data.password, key_id, ciphertext)) {
    (void)connection->RollbackTransaction();
    return Result<void>{ErrorCode::kInternalError, "encrypt credential failed"};
  }

  // 3. 写入 trading_account
  if (const auto insert_account = trading_dao.Insert(*connection, {server_data.account});
      insert_account.error_code != ErrorCode::kSuccess) {
    (void)connection->RollbackTransaction();
    return Result<void>{insert_account.error_code, insert_account.error_message};
  }

  // 4. 写入 account_credential
  qtrade::framework::dao::AccountCredentialRecord credential_row;
  credential_row.tenant_id = server_data.account.tenant_id;
  credential_row.account_id = server_data.account.account_id;
  credential_row.credential_type = qtrade::framework::dao::CredentialType::kPassword;
  credential_row.key_id = key_id;
  credential_row.ciphertext = ciphertext;
  if (const auto insert_credential = credential_dao.Insert(*connection, {credential_row});
      insert_credential.error_code != ErrorCode::kSuccess) {
    (void)connection->RollbackTransaction();
    return Result<void>{insert_credential.error_code, insert_credential.error_message};
  }

  // 5. 提交事务
  if (!connection->CommitTransaction()) {
    (void)connection->RollbackTransaction();
    return Result<void>{ErrorCode::kSystemError, "commit database transaction failed"};
  }
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> AddAccountHandler::VerifyExecutionEffective(AddAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

void AddAccountHandler::Rollback(AddAccountServerData& server_data) {
  (void)server_data;
}

Result<void> AddAccountHandler::NotifyService(AddAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> AddAccountHandler::BuildResponse(AddAccountServerData& server_data,
                                              qtrade::account::v1::AddAccountResponse* response) {
  (void)server_data;
  (void)response;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

}  // namespace qtrade::service
