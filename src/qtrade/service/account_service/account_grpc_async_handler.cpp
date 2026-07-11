/// @file      account_grpc_async_handler.cpp
/// @brief     AccountService Async + CQ RPC 实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/account_grpc_async_handler.hpp"

#include "qtrade_framework/common/grpc/unary_call_tag.hpp"

#include <grpcpp/grpcpp.h>

namespace qtrade::service {
namespace detail {

grpc::Status ToGrpcStatus(ErrorCode code) {
  switch (code) {
    case ErrorCode::kSuccess:
      return grpc::Status::OK;
    case ErrorCode::kNotFound:
      return grpc::Status(grpc::StatusCode::NOT_FOUND, "not found");
    case ErrorCode::kInternal:
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid argument");
    default:
      return grpc::Status(grpc::StatusCode::INTERNAL, "internal error");
  }
}

template <typename Request, typename Response>
using AccountUnaryCallTag = qtrade::common::grpc_async::
  UnaryCallTag<qtrade::account::v1::AccountService::AsyncService, AccountGrpcAsyncHandler, Request, Response>;

}  // namespace detail

AccountGrpcAsyncHandler::AccountGrpcAsyncHandler() = default;

AccountGrpcAsyncHandler::~AccountGrpcAsyncHandler() {
  Shutdown();
}

void AccountGrpcAsyncHandler::Init(qtrade::account::v1::AccountService::AsyncService* async_service,
                                   grpc::ServerCompletionQueue* cq,
                                   std::shared_ptr<IAccountRepository> repository) {
  async_service_ = async_service;
  cq_ = cq;
  repository_ = std::move(repository);
}

void AccountGrpcAsyncHandler::Start() {
  if (started_ || async_service_ == nullptr || cq_ == nullptr || !repository_) {
    return;
  }

  SpawnAddAccount();
  SpawnGetAccount();
  SpawnListAccounts();
  SpawnUpdateAccount();
  SpawnGetCredential();

  started_ = true;
}

void AccountGrpcAsyncHandler::Shutdown() {
  started_ = false;
}

void AccountGrpcAsyncHandler::SpawnAddAccount() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::AddAccountRequest;
  using Response = qtrade::account::v1::AddAccountResponse;
  new detail::AccountUnaryCallTag<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestAddAccount,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response*) {
      return detail::ToGrpcStatus(handler->HandleAddAccount(request));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnAddAccount(); });
}

void AccountGrpcAsyncHandler::SpawnGetAccount() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::GetAccountRequest;
  using Response = qtrade::account::v1::GetAccountResponse;
  new detail::AccountUnaryCallTag<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestGetAccount,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response* response) {
      return detail::ToGrpcStatus(handler->HandleGetAccount(request, *response));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnGetAccount(); });
}

void AccountGrpcAsyncHandler::SpawnListAccounts() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::ListAccountsRequest;
  using Response = qtrade::account::v1::ListAccountsResponse;
  new detail::AccountUnaryCallTag<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestListAccounts,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response* response) {
      return detail::ToGrpcStatus(handler->HandleListAccounts(request, *response));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnListAccounts(); });
}

void AccountGrpcAsyncHandler::SpawnUpdateAccount() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::UpdateAccountRequest;
  using Response = qtrade::account::v1::UpdateAccountResponse;
  new detail::AccountUnaryCallTag<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestUpdateAccount,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response*) {
      return detail::ToGrpcStatus(handler->HandleUpdateAccount(request));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnUpdateAccount(); });
}

void AccountGrpcAsyncHandler::SpawnGetCredential() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::GetCredentialRequest;
  using Response = qtrade::account::v1::GetCredentialResponse;
  new detail::AccountUnaryCallTag<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestGetCredential,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response* response) {
      return detail::ToGrpcStatus(handler->HandleGetCredential(request, *response));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnGetCredential(); });
}

ErrorCode AccountGrpcAsyncHandler::HandleAddAccount(const qtrade::account::v1::AddAccountRequest& request) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }
  if (!request.has_account()) {
    return ErrorCode::kInternal;
  }
  return repository_->AddAccount(request.account());
}

ErrorCode AccountGrpcAsyncHandler::HandleGetAccount(const qtrade::account::v1::GetAccountRequest& request,
                                                    qtrade::account::v1::GetAccountResponse& response) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }

  qtrade::account::v1::TradingAccount account;
  const auto rc = repository_->GetAccount(request.tenant_id(), request.account_id(), account);
  if (rc != ErrorCode::kSuccess) {
    return rc;
  }

  StripAccountPassword(account);
  *response.mutable_account() = std::move(account);
  return ErrorCode::kSuccess;
}

ErrorCode AccountGrpcAsyncHandler::HandleListAccounts(const qtrade::account::v1::ListAccountsRequest& request,
                                                      qtrade::account::v1::ListAccountsResponse& response) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }

  std::vector<qtrade::account::v1::TradingAccount> accounts;
  const auto rc = repository_->ListAccounts(request.tenant_id(), accounts);
  if (rc != ErrorCode::kSuccess) {
    return rc;
  }

  for (auto& account : accounts) {
    StripAccountPassword(account);
    *response.add_accounts() = std::move(account);
  }
  return ErrorCode::kSuccess;
}

ErrorCode AccountGrpcAsyncHandler::HandleUpdateAccount(const qtrade::account::v1::UpdateAccountRequest& request) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }
  if (!request.has_account()) {
    return ErrorCode::kInternal;
  }
  return repository_->UpdateAccount(request.account());
}

ErrorCode AccountGrpcAsyncHandler::HandleGetCredential(const qtrade::account::v1::GetCredentialRequest& request,
                                                       qtrade::account::v1::GetCredentialResponse& response) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }
  return repository_->GetCredential(request.tenant_id(), request.engine_id(), request.account_id(), response);
}

}  // namespace qtrade::service
