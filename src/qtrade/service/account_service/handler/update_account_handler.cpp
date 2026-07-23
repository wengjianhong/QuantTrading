/// @file      update_account_handler.cpp
/// @brief     UpdateAccount：更新 trading_account，password 非空时同步更新 credential
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/update_account_handler.hpp"

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

Result<UpdateAccountServerData> UpdateAccountHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::UpdateAccountRequest* request) {
  (void)context;
  if (!request->has_account()) {
    return Result<UpdateAccountServerData>{ErrorCode::kInternalError, "account is missing"};
  }

  UpdateAccountServerData data;
  data.account = ToTradingAccountRecord(request->account());
  data.password = request->account().password();
  data.update_password = !data.password.empty();
  return {ErrorCode::kSuccess, "success", std::move(data)};
}

Result<void> UpdateAccountHandler::ValidateParams(UpdateAccountServerData& server_data) {
  if (OptionalStringEmpty(server_data.account.tenant_id) || OptionalStringEmpty(server_data.account.account_id)) {
    return Result<void>{ErrorCode::kInternalError, "tenant_id and account_id are required"};
  }
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> UpdateAccountHandler::CheckPreconditions(UpdateAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> UpdateAccountHandler::ExecuteBusiness(UpdateAccountServerData& server_data) {
  auto connection = pool_manager_.Acquire(qtrade::framework::dao::kAccountDatabaseName);
  if (connection == nullptr) {
    return Result<void>{ErrorCode::kSystemError, "database connection pool is unavailable"};
  }
  if (!connection->BeginTransaction()) {
    return Result<void>{ErrorCode::kSystemError, "begin database transaction failed"};
  }

  auto& trading_dao = dao_manager_.Get<qtrade::framework::dao::TradingAccount>();
  auto& credential_dao = dao_manager_.Get<qtrade::framework::dao::AccountCredential>();

  qtrade::framework::dao::TradingAccountRecord where;
  where.tenant_id = server_data.account.tenant_id;
  where.account_id = server_data.account.account_id;

  // 1. 更新 trading_account
  const auto update_result = trading_dao.Update(*connection, server_data.account, where);
  if (update_result.error_code != ErrorCode::kSuccess) {
    (void)connection->RollbackTransaction();
    return Result<void>{update_result.error_code, update_result.error_message};
  }
  if (!update_result.data.has_value() || update_result.data.value() == 0) {
    (void)connection->RollbackTransaction();
    return Result<void>{ErrorCode::kNotFound, "account not found"};
  }

  // 2. password 非空时，同步更新 account_credential
  if (server_data.update_password) {
    std::string key_id;
    std::string ciphertext;
    if (!EncryptCredential(server_data.password, key_id, ciphertext)) {
      (void)connection->RollbackTransaction();
      return Result<void>{ErrorCode::kInternalError, "encrypt credential failed"};
    }

    qtrade::framework::dao::AccountCredentialRecord credential_key;
    credential_key.tenant_id = server_data.account.tenant_id;
    credential_key.account_id = server_data.account.account_id;
    credential_key.credential_type = qtrade::framework::dao::CredentialType::kPassword;
    const auto existing = credential_dao.Select(*connection, credential_key);
    if (existing.error_code != ErrorCode::kSuccess || !existing.data.has_value() || existing.data->empty()) {
      (void)connection->RollbackTransaction();
      return Result<void>{ErrorCode::kNotFound, "credential not found"};
    }

    qtrade::framework::dao::AccountCredentialRecord credential_row;
    credential_row.tenant_id = server_data.account.tenant_id;
    credential_row.account_id = server_data.account.account_id;
    credential_row.credential_type = qtrade::framework::dao::CredentialType::kPassword;
    credential_row.key_id = key_id;
    credential_row.ciphertext = ciphertext;

    const auto update_credential = credential_dao.Update(*connection, credential_row, credential_key);
    if (update_credential.error_code != ErrorCode::kSuccess) {
      (void)connection->RollbackTransaction();
      return Result<void>{update_credential.error_code, update_credential.error_message};
    }
    if (!update_credential.data.has_value() || update_credential.data.value() == 0) {
      (void)connection->RollbackTransaction();
      return Result<void>{ErrorCode::kNotFound, "credential not found"};
    }
  }

  // 3. 提交事务
  if (!connection->CommitTransaction()) {
    (void)connection->RollbackTransaction();
    return Result<void>{ErrorCode::kSystemError, "commit database transaction failed"};
  }
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> UpdateAccountHandler::VerifyExecutionEffective(UpdateAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

void UpdateAccountHandler::Rollback(UpdateAccountServerData& server_data) {
  (void)server_data;
}

Result<void> UpdateAccountHandler::NotifyService(UpdateAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> UpdateAccountHandler::BuildResponse(UpdateAccountServerData& server_data,
                                                 qtrade::account::v1::UpdateAccountResponse* response) {
  (void)server_data;
  (void)response;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

}  // namespace qtrade::service
