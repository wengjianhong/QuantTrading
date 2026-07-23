/// @file      list_accounts_handler.hpp
/// @brief     ListAccounts gRPC 处理器：查询交易账户列表
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_HANDLER_LIST_ACCOUNTS_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_HANDLER_LIST_ACCOUNTS_HANDLER_HPP_

#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <qtrade/dao/account_service/trading_account.hpp>
#include <qtrade/dao/dao_manager.hpp>
#include <qtrade/grpc/grpc_handler_interface.hpp>
#include <qtrade/proto/account/v1/account.pb.h>

#include <string>
#include <vector>

namespace qtrade::service {
using qtrade::framework::dao::TradingAccountRecord;

/// @brief ListAccounts 管道内业务数据
struct ListAccountsServerData {
  /// 租户 ID（空表示不过滤）
  std::string tenant_id;
  /// 查询结果列表
  std::vector<TradingAccountRecord> accounts;
};

/// @brief 查询交易账户列表（password 均置空）
class ListAccountsHandler final
  : public qtrade::framework::grpc::GrpcHandlerInterface<qtrade::account::v1::ListAccountsRequest,
                                                         qtrade::account::v1::ListAccountsResponse,
                                                         ListAccountsServerData> {
 public:
  ListAccountsHandler(const std::string& method_name,
                      qtrade::framework::dao::DbConnectionPoolManager& pool_manager,
                      qtrade::framework::dao::DaoManager& dao_manager)
    : GrpcHandlerInterface(method_name), pool_manager_(pool_manager), dao_manager_(dao_manager) {}
  ~ListAccountsHandler() noexcept override = default;

 protected:
  /// 步骤1: 将 gRPC 请求转为业务数据
  Result<ListAccountsServerData> ConvertToServerData(::grpc::ServerContext* context,
                                                     const qtrade::account::v1::ListAccountsRequest* request) override;

  /// 步骤2: 校验参数合法性
  Result<void> ValidateParams(ListAccountsServerData& server_data) override;

  /// 步骤3: 检查前置条件
  Result<void> CheckPreconditions(ListAccountsServerData& server_data) override;

  /// 步骤4: 执行业务逻辑（取连接并查 trading_account 列表）
  Result<void> ExecuteBusiness(ListAccountsServerData& server_data) override;

  /// 步骤5: 校验操作是否真正生效
  Result<void> VerifyExecutionEffective(ListAccountsServerData& server_data) override;

  /// 步骤6: 失败回滚
  void Rollback(ListAccountsServerData& server_data) override;

  /// 步骤7: 通知其他服务（失败不回滚）
  Result<void> NotifyService(ListAccountsServerData& server_data) override;

  /// 步骤8: 构造响应
  Result<void> BuildResponse(ListAccountsServerData& server_data,
                             qtrade::account::v1::ListAccountsResponse* response) override;

 private:
  qtrade::framework::dao::DbConnectionPoolManager& pool_manager_;
  qtrade::framework::dao::DaoManager& dao_manager_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_HANDLER_LIST_ACCOUNTS_HANDLER_HPP_
