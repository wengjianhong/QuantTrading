/// @file      get_account_handler.cpp
/// @brief     GetAccount：按 tenant_id + account_id 查 trading_account
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/get_account_handler.hpp"

#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/service/account_service/logic/trading_account_converter.hpp"

namespace qtrade::service {

namespace {

using qtrade::framework::grpc::detail::ErrResult;
using qtrade::framework::grpc::detail::OkResult;

}  // namespace

Result<void> GetAccountHandler::Run(::grpc::ServerContext* context,
                                    const qtrade::account::v1::GetAccountRequest* request,
                                    qtrade::account::v1::GetAccountResponse* response) {
  connection_ = pool_manager_.Acquire();
  if (connection_ == nullptr) return ErrResult(ErrorCode::kSystemError, "database connection pool is unavailable");
  const auto result = GrpcHandlerInterface::Run(context, request, response);
  connection_.reset();
  return result;
}

Result<GetAccountServerData> GetAccountHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::GetAccountRequest* request) {
  (void)context;
  GetAccountServerData data;
  data.tenant_id = request->tenant_id();
  data.account_id = request->account_id();
  return OkResult(std::move(data));
}

Result<void> GetAccountHandler::ValidateParams(GetAccountServerData& server_data) {
  if (server_data.tenant_id.empty() || server_data.account_id.empty()) {
    return ErrResult(ErrorCode::kInternal, "tenant_id and account_id are required");
  }
  return OkResult();
}

Result<void> GetAccountHandler::CheckPreconditions(GetAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> GetAccountHandler::ExecuteBusiness(GetAccountServerData& server_data) {
  qtrade::framework::dao::TradingAccountRecord where;
  where.tenant_id = server_data.tenant_id;
  where.account_id = server_data.account_id;

  const auto result = dao_manager_.Get<qtrade::framework::dao::TradingAccount>().Select(*connection_, where);
  if (result.error_code != ErrorCode::kSuccess) {
    return ErrResult(result.error_code, result.error_message);
  }
  if (!result.data.has_value() || result.data->empty()) {
    return ErrResult(ErrorCode::kNotFound, "account not found");
  }

  server_data.account = result.data->front();
  return OkResult();
}

Result<void> GetAccountHandler::VerifyExecutionEffective(GetAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

void GetAccountHandler::Rollback(GetAccountServerData& server_data) {
  (void)server_data;
}

Result<void> GetAccountHandler::NotifyService(GetAccountServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> GetAccountHandler::BuildResponse(GetAccountServerData& server_data,
                                              qtrade::account::v1::GetAccountResponse* response) {
  qtrade::account::v1::TradingAccount account_proto;
  ToTradingAccountProto(server_data.account, account_proto);
  /// 响应中不返回密码
  account_proto.set_password("");
  *response->mutable_account() = std::move(account_proto);
  return OkResult();
}

}  // namespace qtrade::service
