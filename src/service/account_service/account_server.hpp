/// @file      account_server.hpp
/// @brief     account-service gRPC 进程封装
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_SERVER_HPP_
#define QTRADE_SERVICE_ACCOUNT_SERVER_HPP_

#include "service/account_service/repository/account_repository.hpp"

#include <qtrade/proto/account/v1/account.grpc.pb.h>
#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>

namespace qtrade::common::grpc_async {
class GrpcAsyncServer;
}

namespace qtrade::service {

class AccountGrpcAsyncHandler;

struct AccountServiceContext {
  std::shared_ptr<IAccountRepository> repository;
};

class AccountServer {
 public:
  AccountServer();
  ~AccountServer();

  AccountServer(const AccountServer&) = delete;
  AccountServer& operator=(const AccountServer&) = delete;

  ErrorCode Start(const std::string& listen_address, const AccountServiceContext& context);
  void Shutdown();
  void Wait();

  [[nodiscard]] bool IsRunning() const { return running_; }

 private:
  qtrade::account::v1::AccountService::AsyncService async_service_;
  std::unique_ptr<qtrade::common::grpc_async::GrpcAsyncServer> grpc_server_;
  std::unique_ptr<AccountGrpcAsyncHandler> handler_;
  std::shared_ptr<IAccountRepository> repository_;
  bool running_ = false;
};

[[nodiscard]] AccountServiceContext BootstrapAccountService(const std::string& json_path);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_SERVER_HPP_
