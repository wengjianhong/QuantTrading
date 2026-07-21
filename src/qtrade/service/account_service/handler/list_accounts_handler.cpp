/// @file      list_accounts_handler.cpp
/// @brief     ListAccounts：按 tenant_id 可选过滤，返回账户列表
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/list_accounts_handler.hpp"

#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/dao/dao_define.hpp"
#include "qtrade/service/account_service/logic/trading_account_converter.hpp"

#include <utility>

namespace qtrade::service {

Result<ListAccountsServerData> ListAccountsHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::ListAccountsRequest* request) {
  (void)context;
  ListAccountsServerData data;
  data.tenant_id = request->tenant_id();
  return {ErrorCode::kSuccess, "success", std::move(data)};
}

Result<void> ListAccountsHandler::ValidateParams(ListAccountsServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> ListAccountsHandler::CheckPreconditions(ListAccountsServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> ListAccountsHandler::ExecuteBusiness(ListAccountsServerData& server_data) {
  auto connection = pool_manager_.Acquire(qtrade::framework::dao::kAccountDatabaseName);
  if (connection == nullptr) {
    return Result<void>{ErrorCode::kSystemError, "database connection pool is unavailable"};
  }

  qtrade::framework::dao::TradingAccountRecord where;
  if (!server_data.tenant_id.empty()) {
    where.tenant_id = server_data.tenant_id;
  }

  const auto result = dao_manager_.Get<qtrade::framework::dao::TradingAccount>().Select(*connection, where);
  if (result.error_code != ErrorCode::kSuccess || !result.data.has_value()) {
    return Result<void>{result.error_code, result.error_message};
  }

  server_data.accounts = std::move(*result.data);
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> ListAccountsHandler::VerifyExecutionEffective(ListAccountsServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

void ListAccountsHandler::Rollback(ListAccountsServerData& server_data) {
  (void)server_data;
}

Result<void> ListAccountsHandler::NotifyService(ListAccountsServerData& server_data) {
  (void)server_data;
  return Result<void>{ErrorCode::kSuccess, "success"};
}

Result<void> ListAccountsHandler::BuildResponse(ListAccountsServerData& server_data,
                                                qtrade::account::v1::ListAccountsResponse* response) {
  for (const auto& row : server_data.accounts) {
    qtrade::account::v1::TradingAccount account_proto;
    ToTradingAccountProto(row, account_proto);
    /// 列表响应中不返回密码
    account_proto.set_password("");
    *response->add_accounts() = std::move(account_proto);
  }
  return Result<void>{ErrorCode::kSuccess, "success"};
}

}  // namespace qtrade::service
