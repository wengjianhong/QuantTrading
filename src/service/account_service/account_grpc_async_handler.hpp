/// @file      account_grpc_async_handler.hpp
/// @brief     AccountService Async + CQ RPC 处理器
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_GRPC_ASYNC_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_GRPC_ASYNC_HANDLER_HPP_

#include "service/account_service/repository/account_repository.hpp"

#include <qtrade/proto/account/v1/account.grpc.pb.h>

#include <memory>
#include <string>

namespace grpc {
class ServerCompletionQueue;
}

namespace qtrade::service {

class AccountGrpcAsyncHandler {
 public:
  using RepositoryT = IAccountRepository;

  AccountGrpcAsyncHandler();
  ~AccountGrpcAsyncHandler();

  AccountGrpcAsyncHandler(const AccountGrpcAsyncHandler&) = delete;
  AccountGrpcAsyncHandler& operator=(const AccountGrpcAsyncHandler&) = delete;

  void Init(qtrade::account::v1::AccountService::AsyncService* async_service,
            grpc::ServerCompletionQueue* cq,
            std::shared_ptr<IAccountRepository> repository);

  void Start();
  void Shutdown();

  void SpawnAddAccount();
  void SpawnGetAccount();
  void SpawnListAccounts();
  void SpawnUpdateAccount();
  void SpawnGetCredential();

  [[nodiscard]] std::shared_ptr<IAccountRepository> Repository() const { return repository_; }
  [[nodiscard]] grpc::ServerCompletionQueue* CompletionQueue() const { return cq_; }
  [[nodiscard]] qtrade::account::v1::AccountService::AsyncService* AsyncService() const { return async_service_; }

  ErrorCode HandleAddAccount(const qtrade::account::v1::AddAccountRequest& request);
  ErrorCode HandleGetAccount(const qtrade::account::v1::GetAccountRequest& request,
                             qtrade::account::v1::GetAccountResponse& response);
  ErrorCode HandleListAccounts(const qtrade::account::v1::ListAccountsRequest& request,
                               qtrade::account::v1::ListAccountsResponse& response);
  ErrorCode HandleUpdateAccount(const qtrade::account::v1::UpdateAccountRequest& request);
  ErrorCode HandleGetCredential(const qtrade::account::v1::GetCredentialRequest& request,
                                qtrade::account::v1::GetCredentialResponse& response);

 private:
  qtrade::account::v1::AccountService::AsyncService* async_service_ = nullptr;
  grpc::ServerCompletionQueue* cq_ = nullptr;
  std::shared_ptr<IAccountRepository> repository_;
  bool started_ = false;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_GRPC_ASYNC_HANDLER_HPP_
