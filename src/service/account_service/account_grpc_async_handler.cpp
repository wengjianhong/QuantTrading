/// @file      account_grpc_async_handler.cpp
/// @brief     AccountService Async + CQ RPC 实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "service/account_service/account_grpc_async_handler.hpp"

#include "common/grpc/call_data_base.hpp"

#include <functional>

#include <grpcpp/grpcpp.h>

namespace qtrade::service {
namespace detail {

template <typename Request, typename Response>
class UnaryCallData final : public qtrade::common::grpc_async::CallDataBase {
 public:
  using RequestMethod = void (qtrade::account::v1::AccountService::AsyncService::*)(
    grpc::ServerContext*, Request*, grpc::ServerAsyncResponseWriter<Response>*, grpc::CompletionQueue*,
    grpc::ServerCompletionQueue*, void*);
  using HandlerFn = std::function<grpc::Status(AccountGrpcAsyncHandler*, const Request&, Response*)>;
  using RespawnFn = std::function<void(AccountGrpcAsyncHandler*)>;

  UnaryCallData(AccountGrpcAsyncHandler* handler,
                qtrade::account::v1::AccountService::AsyncService* service,
                grpc::ServerCompletionQueue* cq,
                RequestMethod request_method,
                HandlerFn handler_fn,
                RespawnFn respawn_fn)
    : handler_(handler),
      service_(service),
      cq_(cq),
      request_method_(request_method),
      handler_fn_(std::move(handler_fn)),
      respawn_fn_(std::move(respawn_fn)),
      responder_(&ctx_) {
    Proceed(true);
  }

  void Proceed(bool ok) override {
    if (!ok) {
      delete this;
      return;
    }

    if (status_ == CallStatus::kCreate) {
      status_ = CallStatus::kProcess;
      (service_->*request_method_)(&ctx_, &request_, &responder_, cq_, cq_, this);
      return;
    }

    if (status_ == CallStatus::kProcess) {
      status_ = CallStatus::kFinish;
      const grpc::Status status = handler_fn_(handler_, request_, &response_);
      responder_.Finish(response_, status, this);
      return;
    }

    respawn_fn_(handler_);
    delete this;
  }

 private:
  enum class CallStatus { kCreate, kProcess, kFinish };

  AccountGrpcAsyncHandler* handler_;
  qtrade::account::v1::AccountService::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  RequestMethod request_method_;
  HandlerFn handler_fn_;
  RespawnFn respawn_fn_;
  grpc::ServerContext ctx_;
  Request request_;
  Response response_;
  grpc::ServerAsyncResponseWriter<Response> responder_;
  CallStatus status_ = CallStatus::kCreate;
};

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

}  // namespace detail

AccountGrpcAsyncHandler::AccountGrpcAsyncHandler() = default;

AccountGrpcAsyncHandler::~AccountGrpcAsyncHandler() { Shutdown(); }

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

  SpawnRegisterAccount();
  SpawnRotateCredential();
  SpawnBindAccountToEngine();
  SpawnListAccounts();
  SpawnResolveCredential();

  started_ = true;
}

void AccountGrpcAsyncHandler::Shutdown() { started_ = false; }

void AccountGrpcAsyncHandler::SpawnRegisterAccount() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::RegisterAccountRequest;
  using Response = qtrade::account::v1::RegisterAccountResponse;
  new detail::UnaryCallData<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestRegisterAccount,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response*) {
      return detail::ToGrpcStatus(handler->HandleRegisterAccount(request));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnRegisterAccount(); });
}

void AccountGrpcAsyncHandler::SpawnRotateCredential() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::RotateCredentialRequest;
  using Response = qtrade::account::v1::RotateCredentialResponse;
  new detail::UnaryCallData<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestRotateCredential,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response*) {
      return detail::ToGrpcStatus(handler->HandleRotateCredential(request));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnRotateCredential(); });
}

void AccountGrpcAsyncHandler::SpawnBindAccountToEngine() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::BindAccountToEngineRequest;
  using Response = qtrade::account::v1::BindAccountToEngineResponse;
  new detail::UnaryCallData<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestBindAccountToEngine,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response*) {
      return detail::ToGrpcStatus(handler->HandleBindAccountToEngine(request));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnBindAccountToEngine(); });
}

void AccountGrpcAsyncHandler::SpawnListAccounts() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::ListAccountsRequest;
  using Response = qtrade::account::v1::ListAccountsResponse;
  new detail::UnaryCallData<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestListAccounts,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response* response) {
      return detail::ToGrpcStatus(handler->HandleListAccounts(request, *response));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnListAccounts(); });
}

void AccountGrpcAsyncHandler::SpawnResolveCredential() {
  if (!async_service_ || !cq_ || !repository_) {
    return;
  }
  using Request = qtrade::account::v1::ResolveCredentialRequest;
  using Response = qtrade::account::v1::ResolveCredentialResponse;
  new detail::UnaryCallData<Request, Response>(
    this,
    async_service_,
    cq_,
    &qtrade::account::v1::AccountService::AsyncService::RequestResolveCredential,
    [](AccountGrpcAsyncHandler* handler, const Request& request, Response* response) {
      return detail::ToGrpcStatus(handler->HandleResolveCredential(request, *response));
    },
    [](AccountGrpcAsyncHandler* handler) { handler->SpawnResolveCredential(); });
}

ErrorCode AccountGrpcAsyncHandler::HandleRegisterAccount(const qtrade::account::v1::RegisterAccountRequest& request) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }
  if (!request.has_account()) {
    return ErrorCode::kInternal;
  }
  return repository_->RegisterAccount(request.account(), request.password());
}

ErrorCode AccountGrpcAsyncHandler::HandleRotateCredential(
  const qtrade::account::v1::RotateCredentialRequest& request) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }
  return repository_->RotateCredential(request.account_id(), request.password());
}

ErrorCode AccountGrpcAsyncHandler::HandleBindAccountToEngine(
  const qtrade::account::v1::BindAccountToEngineRequest& request) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }
  return repository_->BindAccountToEngine(request.account_id(), request.engine_id());
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
    *response.add_accounts() = std::move(account);
  }
  return ErrorCode::kSuccess;
}

ErrorCode AccountGrpcAsyncHandler::HandleResolveCredential(
  const qtrade::account::v1::ResolveCredentialRequest& request,
  qtrade::account::v1::ResolveCredentialResponse& response) {
  if (!repository_) {
    return ErrorCode::kSystemError;
  }
  return repository_->ResolveCredential(request.engine_id(), request.account_id(), response);
}

}  // namespace qtrade::service
