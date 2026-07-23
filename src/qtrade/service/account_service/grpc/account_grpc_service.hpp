/// @file      account_grpc_service.hpp
/// @brief     AccountService 同步 gRPC 实现（Unary RPC 薄路由层）
/// @details   各 RPC 委托 handler/ 下对应处理器，业务逻辑不在此文件实现
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_GRPC_SERVICE_HPP_
#define QTRADE_SERVICE_ACCOUNT_GRPC_SERVICE_HPP_

#include "qtrade/dao/dao_manager.hpp"
#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <qtrade/proto/account/v1/account.grpc.pb.h>

#include <memory>

namespace qtrade::service {

/// @brief 交易账户 gRPC 同步 Service（proto AccountService）
class AccountGrpcService final : public qtrade::account::v1::AccountService::Service {
 public:
  AccountGrpcService(std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection,
                     std::shared_ptr<qtrade::framework::dao::DaoManager> dao);

  /// @brief 添加交易账户
  grpc::Status AddAccount(grpc::ServerContext* context,
                          const qtrade::account::v1::AddAccountRequest* request,
                          qtrade::account::v1::AddAccountResponse* response) override;

  /// @brief 查询单个交易账户（password 置空）
  grpc::Status GetAccount(grpc::ServerContext* context,
                          const qtrade::account::v1::GetAccountRequest* request,
                          qtrade::account::v1::GetAccountResponse* response) override;

  /// @brief 查询交易账户列表（password 均置空）
  grpc::Status ListAccounts(grpc::ServerContext* context,
                            const qtrade::account::v1::ListAccountsRequest* request,
                            qtrade::account::v1::ListAccountsResponse* response) override;

  /// @brief 更新交易账户（password 非空时同步更新凭证）
  grpc::Status UpdateAccount(grpc::ServerContext* context,
                             const qtrade::account::v1::UpdateAccountRequest* request,
                             qtrade::account::v1::UpdateAccountResponse* response) override;

  /// @brief 获取登录凭证（含明文密码，供引擎按需拉取）
  grpc::Status GetCredential(grpc::ServerContext* context,
                             const qtrade::account::v1::GetCredentialRequest* request,
                             qtrade::account::v1::GetCredentialResponse* response) override;

 private:
  /// @brief 检查数据库连接是否就绪
  [[nodiscard]] bool DatabaseReady() const;

  /// 本进程 DaoManager
  std::shared_ptr<qtrade::framework::dao::DaoManager> dao_mgr_;
  /// 数据库连接池管理器
  std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection_pool_mgr_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_GRPC_SERVICE_HPP_
