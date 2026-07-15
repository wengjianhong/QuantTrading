/// @file      account_grpc_service.cpp
/// @brief     AccountService 同步 gRPC 实现（薄路由层）
/// @details   各 RPC 仅负责 DatabaseReady 检查、委托 handler.Run()、映射 grpc::Status
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/grpc/account_grpc_service.hpp"

#include "qtrade/service/account_service/handler/add_account_handler.hpp"
#include "qtrade/service/account_service/handler/get_account_handler.hpp"
#include "qtrade/service/account_service/handler/get_credential_handler.hpp"
#include "qtrade/service/account_service/handler/list_accounts_handler.hpp"
#include "qtrade/service/account_service/handler/update_account_handler.hpp"

#include <qtrade_framework/grpc/grpc_status_utils.hpp>

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::service {

namespace {

using qtrade::framework::grpc::ToGrpcStatus;

}  // namespace

AccountGrpcService::AccountGrpcService(std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection)
  : connection_(std::move(connection)) {}

bool AccountGrpcService::DatabaseReady() const {
  return connection_ != nullptr && connection_->IsReady();
}

grpc::Status AccountGrpcService::AddAccount(grpc::ServerContext* context,
                                            const qtrade::account::v1::AddAccountRequest* request,
                                            qtrade::account::v1::AddAccountResponse* response) {
  if (!DatabaseReady()) {
    return ToGrpcStatus(ErrorCode::kSystemError);
  }

  AddAccountHandler handler("AddAccount");
  const Result<void> result = handler.Run(context, request, response);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::error("AddAccount failed: {}", result.error_message);
  }
  return ToGrpcStatus(result.error_code);
}

grpc::Status AccountGrpcService::GetAccount(grpc::ServerContext* context,
                                            const qtrade::account::v1::GetAccountRequest* request,
                                            qtrade::account::v1::GetAccountResponse* response) {
  if (!DatabaseReady()) {
    return ToGrpcStatus(ErrorCode::kSystemError);
  }

  GetAccountHandler handler("GetAccount");
  const Result<void> result = handler.Run(context, request, response);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::error("GetAccount failed: {}", result.error_message);
  }
  return ToGrpcStatus(result.error_code);
}

grpc::Status AccountGrpcService::ListAccounts(grpc::ServerContext* context,
                                              const qtrade::account::v1::ListAccountsRequest* request,
                                              qtrade::account::v1::ListAccountsResponse* response) {
  if (!DatabaseReady()) {
    return ToGrpcStatus(ErrorCode::kSystemError);
  }

  ListAccountsHandler handler("ListAccounts");
  const Result<void> result = handler.Run(context, request, response);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::error("ListAccounts failed: {}", result.error_message);
  }
  return ToGrpcStatus(result.error_code);
}

grpc::Status AccountGrpcService::UpdateAccount(grpc::ServerContext* context,
                                               const qtrade::account::v1::UpdateAccountRequest* request,
                                               qtrade::account::v1::UpdateAccountResponse* response) {
  if (!DatabaseReady()) {
    return ToGrpcStatus(ErrorCode::kSystemError);
  }

  UpdateAccountHandler handler("UpdateAccount");
  const Result<void> result = handler.Run(context, request, response);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::error("UpdateAccount failed: {}", result.error_message);
  }
  return ToGrpcStatus(result.error_code);
}

grpc::Status AccountGrpcService::GetCredential(grpc::ServerContext* context,
                                               const qtrade::account::v1::GetCredentialRequest* request,
                                               qtrade::account::v1::GetCredentialResponse* response) {
  if (!DatabaseReady()) {
    return ToGrpcStatus(ErrorCode::kSystemError);
  }

  GetCredentialHandler handler("GetCredential");
  const Result<void> result = handler.Run(context, request, response);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::error("GetCredential failed: {}", result.error_message);
  }
  return ToGrpcStatus(result.error_code);
}

}  // namespace qtrade::service
