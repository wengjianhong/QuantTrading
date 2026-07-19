/// @file      update_account_handler.cpp
/// @brief     UpdateAccount：更新 trading_account，password 非空时同步更新 credential
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/update_account_handler.hpp"

#include "qtrade/dao/account_service/account_credential.hpp"
#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/service/account_service/logic/credential_codec.hpp"
#include "qtrade/service/account_service/logic/trading_account_converter.hpp"

namespace qtrade::service {

namespace {

using qtrade::framework::grpc::detail::ErrResult;
using qtrade::framework::grpc::detail::OkResult;

[[nodiscard]] bool OptionalStringEmpty(const std::optional<std::string>& value) {
  return !value.has_value() || value->empty();
}

}  // namespace

Result<void> UpdateAccountHandler::Run(::grpc::ServerContext* context,
                                       const qtrade::account::v1::UpdateAccountRequest* request,
                                       qtrade::account::v1::UpdateAccountResponse* response) {
  connection_ = pool_manager_.Acquire();
  if (connection_ == nullptr) return ErrResult(ErrorCode::kSystemError, "database connection pool is unavailable");
  if (!connection_->BeginTransaction()) return ErrResult(ErrorCode::kSystemError, "begin database transaction failed");
  Result<void> result = GrpcHandlerInterface::Run(context, request, response);
  if (result.error_code == ErrorCode::kSuccess) {
    if (!connection_->CommitTransaction()) {
      (void)connection_->RollbackTransaction();
      result = ErrResult(ErrorCode::kSystemError, "commit database transaction failed");
    }
  } else {
    (void)connection_->RollbackTransaction();
  }
  connection_.reset();
  return result;
}

Result<UpdateAccountServerData> UpdateAccountHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::UpdateAccountRequest* request) {
  (void)context;
  if (!request->has_account()) {
    return ErrResult<UpdateAccountServerData>(ErrorCode::kInternal, "account is missing");
  }

  UpdateAccountServerData data;
  data.account = ToTradingAccountRecord(request->account());
  data.password = request->account().password();
  data.update_password = !data.password.empty();
  return OkResult(std::move(data));
}

Result<void> UpdateAccountHandler::ValidateParams(UpdateAccountServerData& server_data) {
  if (OptionalStringEmpty(server_data.account.tenant_id) || OptionalStringEmpty(server_data.account.account_id)) {
    return ErrResult(ErrorCode::kInternal, "tenant_id and account_id are required");
  }
  return OkResult();
}

Result<void> UpdateAccountHandler::CheckPreconditions(UpdateAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> UpdateAccountHandler::ExecuteBusiness(UpdateAccountServerData& server_data) {
  auto& trading_dao = dao_manager_.Get<qtrade::framework::dao::TradingAccount>();
  auto& credential_dao = dao_manager_.Get<qtrade::framework::dao::AccountCredential>();

  qtrade::framework::dao::TradingAccountRecord where;
  where.tenant_id = server_data.account.tenant_id;
  where.account_id = server_data.account.account_id;

  /// 更新 trading_account
  const auto update_result = trading_dao.Update(*connection_, server_data.account, where);
  if (update_result.error_code != ErrorCode::kSuccess) {
    return ErrResult(update_result.error_code, update_result.error_message);
  }
  if (!update_result.data.has_value() || update_result.data.value() == 0) {
    return ErrResult(ErrorCode::kNotFound, "account not found");
  }

  if (!server_data.update_password) {
    return OkResult();
  }

  /// password 非空时，同步更新 account_credential
  std::string key_id;
  std::string ciphertext;
  if (!EncryptCredential(server_data.password, key_id, ciphertext)) {
    return ErrResult(ErrorCode::kInternal, "encrypt credential failed");
  }

  qtrade::framework::dao::AccountCredentialRecord credential_key;
  credential_key.tenant_id = server_data.account.tenant_id;
  credential_key.account_id = server_data.account.account_id;
  credential_key.credential_type = qtrade::framework::dao::CredentialType::kPassword;
  const auto existing = credential_dao.Select(*connection_, credential_key);
  if (existing.error_code != ErrorCode::kSuccess || !existing.data.has_value() || existing.data->empty()) {
    return ErrResult(ErrorCode::kNotFound, "credential not found");
  }

  qtrade::framework::dao::AccountCredentialRecord credential_row;
  credential_row.tenant_id = server_data.account.tenant_id;
  credential_row.account_id = server_data.account.account_id;
  credential_row.credential_type = qtrade::framework::dao::CredentialType::kPassword;
  credential_row.key_id = key_id;
  credential_row.ciphertext = ciphertext;

  const auto update_credential = credential_dao.Update(*connection_, credential_row, credential_key);
  if (update_credential.error_code != ErrorCode::kSuccess) {
    return ErrResult(update_credential.error_code, update_credential.error_message);
  }
  if (!update_credential.data.has_value() || update_credential.data.value() == 0) {
    return ErrResult(ErrorCode::kNotFound, "credential not found");
  }

  server_data.credential_updated = true;
  return OkResult();
}

Result<void> UpdateAccountHandler::VerifyExecutionEffective(UpdateAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

void UpdateAccountHandler::Rollback(UpdateAccountServerData& server_data) {
  /// 跨表更新无 DB 事务；失败回滚需引入事务后完善。
  (void)server_data;
}

Result<void> UpdateAccountHandler::NotifyService(UpdateAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> UpdateAccountHandler::BuildResponse(UpdateAccountServerData& server_data,
                                                 qtrade::account::v1::UpdateAccountResponse* response) {
  (void)server_data;
  (void)response;
  return OkResult();
}

}  // namespace qtrade::service
