/// @file      get_account_handler.cpp
/// @brief     GetAccount：按 tenant_id + account_id 查 trading_account
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/get_account_handler.hpp"

#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/dao/dao_define.hpp"
#include "qtrade/service/account_service/logic/trading_account_converter.hpp"

namespace qtrade::service {

Result<GetAccountServerData> GetAccountHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::GetAccountRequest* request) {
  (void)context;
  GetAccountServerData data;
  data.tenant_id = request->tenant_id();
  data.account_id = request->account_id();
  return {ErrorCode::kSuccess, "success", std::move(data)};
}

Result<void> GetAccountHandler::ValidateParams(GetAccountServerData& server_data) {
  if (server_data.tenant_id.empty() || server_data.account_id.empty()) {
    return Result<void>{ErrorCode::kInternalError, "tenant_id and account_id are required"};
  }
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetAccountHandler::CheckPreconditions(GetAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetAccountHandler::ExecuteBusiness(GetAccountServerData& server_data) {
  auto connection = pool_manager_.Acquire(qtrade::framework::dao::kAccountDatabaseName);
  if (connection == nullptr) {
    return Result<void>{ErrorCode::kSystemError, "database connection pool is unavailable"};
  }

  qtrade::framework::dao::TradingAccountRecord where;
  where.tenant_id = server_data.tenant_id;
  where.account_id = server_data.account_id;

  const auto result = dao_manager_.Get<qtrade::framework::dao::TradingAccount>().Select(*connection, where);
  if (result.error_code != ErrorCode::kSuccess) {
    return Result<void>{result.error_code, result.error_message};
  }
  if (!result.data.has_value() || result.data->empty()) {
    return Result<void>{ErrorCode::kNotFound, "account not found"};
  }

  server_data.account = result.data->front();
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetAccountHandler::VerifyExecutionEffective(GetAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

void GetAccountHandler::Rollback(GetAccountServerData& server_data) {
  (void)server_data;
}

Result<void> GetAccountHandler::NotifyService(GetAccountServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> GetAccountHandler::BuildResponse(GetAccountServerData& server_data,
                                              qtrade::account::v1::GetAccountResponse* response) {
  qtrade::account::v1::TradingAccount account_proto;
  ToTradingAccountProto(server_data.account, account_proto);
  /// 响应中不返回密码
  account_proto.set_password("");
  *response->mutable_account() = std::move(account_proto);
  return Result<void>{ErrorCode::kSuccess, "success"};
}

}  // namespace qtrade::service
