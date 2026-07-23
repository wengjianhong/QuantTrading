/// @file      get_credential_handler.cpp
/// @brief     GetCredential：校验账户状态后解密并返回登录凭证
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/get_credential_handler.hpp"

#include "qtrade/dao/account_service/account_credential.hpp"
#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/dao/dao_define.hpp"
#include "qtrade/service/account_service/logic/credential_codec.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::service {

Result<GetCredentialServerData> GetCredentialHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::GetCredentialRequest* request) {
  (void)context;
  GetCredentialServerData data;
  data.tenant_id = request->tenant_id();
  data.account_id = request->account_id();
  data.engine_id = request->engine_id();
  return {ErrorCode::kSuccess, "success", std::move(data)};
}

Result<void> GetCredentialHandler::ValidateParams(GetCredentialServerData& server_data) {
  if (server_data.tenant_id.empty() || server_data.engine_id.empty() || server_data.account_id.empty()) {
    return Result<void>{ErrorCode::kInternalError, "tenant_id, engine_id and account_id are required"};
  }
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetCredentialHandler::CheckPreconditions(GetCredentialServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetCredentialHandler::ExecuteBusiness(GetCredentialServerData& server_data) {
  auto connection = pool_manager_.Acquire(qtrade::framework::dao::kAccountDatabaseName);
  if (connection == nullptr) {
    return Result<void>{ErrorCode::kSystemError, "database connection pool is unavailable"};
  }

  qtrade::framework::dao::TradingAccountRecord where;
  where.tenant_id = server_data.tenant_id;
  where.account_id = server_data.account_id;

  /// 查询 trading_account
  const auto account_result = dao_manager_.Get<qtrade::framework::dao::TradingAccount>().Select(*connection, where);
  if (account_result.error_code != ErrorCode::kSuccess) {
    return Result<void>{account_result.error_code, account_result.error_message};
  }
  if (!account_result.data.has_value() || account_result.data->empty()) {
    return Result<void>{ErrorCode::kNotFound, "account not found"};
  }

  server_data.account = account_result.data->front();
  if (server_data.account.status.value_or("") == "disabled") {
    return Result<void>{ErrorCode::kInternalError, "account is disabled"};
  }

  /// 查询并解密 account_credential（默认取交易密码）
  qtrade::framework::dao::AccountCredentialRecord cred_where;
  cred_where.tenant_id = server_data.tenant_id;
  cred_where.account_id = server_data.account_id;
  cred_where.credential_type = qtrade::framework::dao::CredentialType::kPassword;
  const auto cred_result =
    dao_manager_.Get<qtrade::framework::dao::AccountCredential>().Select(*connection, cred_where);
  if (cred_result.error_code != ErrorCode::kSuccess) {
    return Result<void>{cred_result.error_code, cred_result.error_message};
  }
  if (!cred_result.data.has_value() || cred_result.data->empty()) {
    return Result<void>{ErrorCode::kNotFound, "credential not found"};
  }

  const auto& cred_row = cred_result.data->front();
  if (!cred_row.key_id.has_value() || !cred_row.ciphertext.has_value()) {
    return Result<void>{ErrorCode::kInternalError, "credential data invalid"};
  }

  if (!DecryptCredential(cred_row.key_id.value(), cred_row.ciphertext.value(), server_data.password)) {
    return Result<void>{ErrorCode::kInternalError, "decrypt credential failed"};
  }

  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetCredentialHandler::VerifyExecutionEffective(GetCredentialServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

void GetCredentialHandler::Rollback(GetCredentialServerData& server_data) {
  (void)server_data;
}

Result<void> GetCredentialHandler::NotifyService(GetCredentialServerData& server_data) {
  /// 记录凭证拉取审计日志
  spdlog::info("[GetCredentialHandler] credential fetched for tenant={} engine={} account={}",
               server_data.tenant_id,
               server_data.engine_id,
               server_data.account_id);
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetCredentialHandler::BuildResponse(GetCredentialServerData& server_data,
                                                 qtrade::account::v1::GetCredentialResponse* response) {
  auto* credential = response->mutable_credential();
  credential->set_tenant_id(server_data.tenant_id);
  credential->set_account_id(server_data.account_id);
  credential->set_broker_id(server_data.account.broker_id.value_or(""));
  credential->set_connection_string(server_data.account.connection_string.value_or(""));
  credential->set_password(server_data.password);
  return Result<void>{ErrorCode::kSuccess, "success"};
}

}  // namespace qtrade::service
