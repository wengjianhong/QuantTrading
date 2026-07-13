/// @file      add_account_handler.cpp
/// @brief     AddAccount：写入 trading_account 与 account_credential
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/add_account_handler.hpp"

#include "qtrade/dao/account_credential.hpp"
#include "qtrade/dao/trading_account.hpp"
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

Result<AddAccountServerData> AddAccountHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::AddAccountRequest* request) {
  (void)context;
  if (!request->has_account()) {
    return ErrResult<AddAccountServerData>(ErrorCode::kInternal, "account is missing");
  }

  AddAccountServerData data;
  data.account = ToTradingAccountRecord(request->account());
  data.password = request->account().password();
  return OkResult(std::move(data));
}

Result<void> AddAccountHandler::ValidateParams(AddAccountServerData& server_data) {
  if (OptionalStringEmpty(server_data.account.tenant_id) || OptionalStringEmpty(server_data.account.account_id) ||
      server_data.password.empty()) {
    return ErrResult(ErrorCode::kInternal, "tenant_id, account_id and password are required");
  }
  return OkResult();
}

Result<void> AddAccountHandler::CheckPreconditions(AddAccountServerData& server_data) {
  qtrade::framework::dao::TradingAccountRecord key;
  key.tenant_id = server_data.account.tenant_id;
  key.account_id = server_data.account.account_id;

  const auto exists = qtrade::framework::dao::TradingAccount::Instance().Count(key);
  if (exists.error_code != ErrorCode::kSuccess) {
    return ErrResult(exists.error_code, exists.error_message);
  }
  if (exists.data.has_value() && exists.data.value() > 0) {
    return ErrResult(ErrorCode::kSystemError, "account already exists");
  }
  return OkResult();
}

Result<void> AddAccountHandler::ExecuteBusiness(AddAccountServerData& server_data) {
  auto& trading_dao = qtrade::framework::dao::TradingAccount::Instance();
  auto& credential_dao = qtrade::framework::dao::AccountCredential::Instance();

  /// 加密明文密码
  std::string key_id;
  std::string ciphertext;
  if (!EncryptCredential(server_data.password, key_id, ciphertext)) {
    return ErrResult(ErrorCode::kInternal, "encrypt credential failed");
  }

  /// 写入 trading_account
  if (const auto insert_account = trading_dao.Insert({server_data.account});
      insert_account.error_code != ErrorCode::kSuccess) {
    return ErrResult(insert_account.error_code, insert_account.error_message);
  }
  server_data.account_inserted = true;

  /// 写入 account_credential
  qtrade::framework::dao::AccountCredentialRecord credential_row;
  credential_row.tenant_id = server_data.account.tenant_id;
  credential_row.account_id = server_data.account.account_id;
  credential_row.key_id = key_id;
  credential_row.ciphertext = ciphertext;
  credential_row.version = 1;

  if (const auto insert_credential = credential_dao.Insert({credential_row});
      insert_credential.error_code != ErrorCode::kSuccess) {
    return ErrResult(insert_credential.error_code, insert_credential.error_message);
  }

  return OkResult();
}

Result<void> AddAccountHandler::VerifyExecutionEffective(AddAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

void AddAccountHandler::Rollback(AddAccountServerData& server_data) {
  if (!server_data.account_inserted) {
    return;
  }

  /// 凭证写入失败时，删除已插入的 trading_account 记录
  qtrade::framework::dao::TradingAccountRecord where;
  where.tenant_id = server_data.account.tenant_id;
  where.account_id = server_data.account.account_id;
  (void)qtrade::framework::dao::TradingAccount::Instance().Delete(where);
  server_data.account_inserted = false;
}

Result<void> AddAccountHandler::NotifyService(AddAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> AddAccountHandler::BuildResponse(AddAccountServerData& server_data,
                                              qtrade::account::v1::AddAccountResponse* response) {
  (void)server_data;
  (void)response;
  return OkResult();
}

}  // namespace qtrade::service
