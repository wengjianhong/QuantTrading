/// @file      list_accounts_handler.cpp
/// @brief     ListAccounts：按 tenant_id 可选过滤，返回账户列表
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/handler/list_accounts_handler.hpp"

#include "qtrade/service/account_service/logic/trading_account_converter.hpp"
#include "qtrade_framework/dao/trading_account.hpp"

#include <utility>

namespace qtrade::service {

namespace {

using qtrade::framework::grpc::detail::ErrResult;
using qtrade::framework::grpc::detail::OkResult;

}  // namespace

Result<ListAccountsServerData> ListAccountsHandler::ConvertToServerData(
  ::grpc::ServerContext* context, const qtrade::account::v1::ListAccountsRequest* request) {
  (void)context;
  ListAccountsServerData data;
  data.tenant_id = request->tenant_id();
  return OkResult(std::move(data));
}

Result<void> ListAccountsHandler::ValidateParams(ListAccountsServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> ListAccountsHandler::CheckPreconditions(ListAccountsServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> ListAccountsHandler::ExecuteBusiness(ListAccountsServerData& server_data) {
  qtrade::framework::dao::TradingAccountRecord where;
  if (!server_data.tenant_id.empty()) {
    where.tenant_id = server_data.tenant_id;
  }

  const auto result = qtrade::framework::dao::TradingAccount::Instance().Select(where);
  if (result.error_code != ErrorCode::kSuccess || !result.data.has_value()) {
    return ErrResult(result.error_code, result.error_message);
  }

  server_data.accounts.reserve(result.data->size());
  for (const auto& row : *result.data) {
    qtrade::account::v1::TradingAccount account;
    ToTradingAccountProto(row, account);
    /// 列表响应中不返回密码
    account.set_password("");
    server_data.accounts.push_back(std::move(account));
  }
  return OkResult();
}

Result<void> ListAccountsHandler::VerifyExecutionEffective(ListAccountsServerData& server_data) {
  (void)server_data;
  return OkResult();
}

void ListAccountsHandler::Rollback(ListAccountsServerData& server_data) {
  (void)server_data;
}

Result<void> ListAccountsHandler::NotifyService(ListAccountsServerData& server_data) {
  (void)server_data;
  return OkResult();
}

Result<void> ListAccountsHandler::BuildResponse(ListAccountsServerData& server_data,
                                                qtrade::account::v1::ListAccountsResponse* response) {
  for (auto& account : server_data.accounts) {
    *response->add_accounts() = std::move(account);
  }
  return OkResult();
}

}  // namespace qtrade::service
