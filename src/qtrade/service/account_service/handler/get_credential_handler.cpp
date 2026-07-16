/// @file      get_credential_handler.cpp
/// @brief     GetCredential：校验账户状态后解密并返回登录凭证
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/get_credential_handler.hpp"

#include "qtrade/dao/account_service/account_credential.hpp"
#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/service/account_service/logic/credential_codec.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::service {

namespace {

using qtrade::framework::grpc::detail::ErrResult;
using qtrade::framework::grpc::detail::OkResult;

}  // namespace

Result<GetCredentialServerData> GetCredentialHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::GetCredentialRequest* request) {
  (void)context;
  GetCredentialServerData data;
  data.tenant_id = request->tenant_id();
  data.account_id = request->account_id();
  data.engine_id = request->engine_id();
  return OkResult(std::move(data));
}

Result<void> GetCredentialHandler::ValidateParams(GetCredentialServerData& server_data) {
  if (server_data.tenant_id.empty() || server_data.engine_id.empty() || server_data.account_id.empty()) {
    return ErrResult(ErrorCode::kInternal, "tenant_id, engine_id and account_id are required");
  }
  return OkResult();
}

Result<void> GetCredentialHandler::CheckPreconditions(GetCredentialServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> GetCredentialHandler::ExecuteBusiness(GetCredentialServerData& server_data) {
  qtrade::framework::dao::TradingAccountRecord where;
  where.tenant_id = server_data.tenant_id;
  where.account_id = server_data.account_id;

  /// 查询 trading_account
  const auto account_result = qtrade::framework::dao::TradingAccount::Instance().Select(where);
  if (account_result.error_code != ErrorCode::kSuccess) {
    return ErrResult(account_result.error_code, account_result.error_message);
  }
  if (!account_result.data.has_value() || account_result.data->empty()) {
    return ErrResult(ErrorCode::kNotFound, "account not found");
  }

  server_data.account = account_result.data->front();
  if (server_data.account.status.value_or("") == "disabled") {
    return ErrResult(ErrorCode::kInternal, "account is disabled");
  }

  /// 查询并解密 account_credential（默认取交易密码）
  qtrade::framework::dao::AccountCredentialRecord cred_where;
  cred_where.tenant_id = server_data.tenant_id;
  cred_where.account_id = server_data.account_id;
  cred_where.credential_type = qtrade::framework::dao::CredentialType::kPassword;
  const auto cred_result = qtrade::framework::dao::AccountCredential::Instance().Select(cred_where);
  if (cred_result.error_code != ErrorCode::kSuccess) {
    return ErrResult(cred_result.error_code, cred_result.error_message);
  }
  if (!cred_result.data.has_value() || cred_result.data->empty()) {
    return ErrResult(ErrorCode::kNotFound, "credential not found");
  }

  const auto& cred_row = cred_result.data->front();
  if (!cred_row.key_id.has_value() || !cred_row.ciphertext.has_value()) {
    return ErrResult(ErrorCode::kInternal, "credential data invalid");
  }

  if (!DecryptCredential(cred_row.key_id.value(), cred_row.ciphertext.value(), server_data.password)) {
    return ErrResult(ErrorCode::kInternal, "decrypt credential failed");
  }

  return OkResult();
}

Result<void> GetCredentialHandler::VerifyExecutionEffective(GetCredentialServerData& server_data) {
  (void)server_data;
  return OkResult();
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
  return OkResult();
}

Result<void> GetCredentialHandler::BuildResponse(GetCredentialServerData& server_data,
                                                 qtrade::account::v1::GetCredentialResponse* response) {
  auto* credential = response->mutable_credential();
  credential->set_tenant_id(server_data.tenant_id);
  credential->set_account_id(server_data.account_id);
  credential->set_broker_id(server_data.account.broker_id.value_or(""));
  credential->set_connection_string(server_data.account.connection_string.value_or(""));
  credential->set_password(server_data.password);
  return OkResult();
}

}  // namespace qtrade::service
