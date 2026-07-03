/// @file      account_grpc_async_handler.hpp
/// @brief     AccountService Async + CQ RPC 处理器
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_GRPC_ASYNC_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_GRPC_ASYNC_HANDLER_HPP_

#include "service/account_service/repository/account_repository.hpp"

#include <qtrade/account/v1/account.grpc.pb.h>

#include <memory>

namespace grpc {
class ServerCompletionQueue;
}

namespace qtrade::service {

class AccountGrpcAsyncHandler {
 public:
  AccountGrpcAsyncHandler();
  ~AccountGrpcAsyncHandler();

  AccountGrpcAsyncHandler(const AccountGrpcAsyncHandler&) = delete;
  AccountGrpcAsyncHandler& operator=(const AccountGrpcAsyncHandler&) = delete;

  void Init(qtrade::account::v1::AccountService::AsyncService* async_service,
            grpc::ServerCompletionQueue* cq,
            std::shared_ptr<IAccountRepository> repository);

  void Start();
  void Shutdown();

  void SpawnRegisterAccount();
  void SpawnRotateCredential();
  void SpawnBindAccountToEngine();
  void SpawnListAccounts();
  void SpawnResolveCredential();

  [[nodiscard]] std::shared_ptr<IAccountRepository> Repository() const { return repository_; }
  [[nodiscard]] grpc::ServerCompletionQueue* CompletionQueue() const { return cq_; }
  [[nodiscard]] qtrade::account::v1::AccountService::AsyncService* AsyncService() const { return async_service_; }

  ErrorCode HandleRegisterAccount(const qtrade::account::v1::RegisterAccountRequest& request);
  ErrorCode HandleRotateCredential(const qtrade::account::v1::RotateCredentialRequest& request);
  ErrorCode HandleBindAccountToEngine(const qtrade::account::v1::BindAccountToEngineRequest& request);
  ErrorCode HandleListAccounts(const qtrade::account::v1::ListAccountsRequest& request,
                               qtrade::account::v1::ListAccountsResponse& response);
  ErrorCode HandleResolveCredential(const qtrade::account::v1::ResolveCredentialRequest& request,
                                    qtrade::account::v1::ResolveCredentialResponse& response);

 private:
  qtrade::account::v1::AccountService::AsyncService* async_service_ = nullptr;
  grpc::ServerCompletionQueue* cq_ = nullptr;
  std::shared_ptr<IAccountRepository> repository_;
  bool started_ = false;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_GRPC_ASYNC_HANDLER_HPP_
