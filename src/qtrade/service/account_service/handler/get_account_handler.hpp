/// @file      get_account_handler.hpp
/// @brief     GetAccount gRPC 处理器：查询单个交易账户
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_HANDLER_GET_ACCOUNT_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_HANDLER_GET_ACCOUNT_HANDLER_HPP_

#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <qtrade/dao/account_service/trading_account.hpp>
#include <qtrade/dao/dao_manager.hpp>
#include <qtrade/grpc/grpc_handler_interface.hpp>
#include <qtrade/proto/account/v1/account.pb.h>

#include <string>

namespace qtrade::service {

/// @brief GetAccount 管道内业务数据
struct GetAccountServerData {
  /// 租户 ID
  std::string tenant_id;
  /// 交易账户号
  std::string account_id;
  /// 查询结果
  qtrade::framework::dao::TradingAccountRecord account;
};

/// @brief 查询单个交易账户（不返回密码）
class GetAccountHandler final
  : public qtrade::framework::grpc::GrpcHandlerInterface<qtrade::account::v1::GetAccountRequest,
                                                         qtrade::account::v1::GetAccountResponse,
                                                         GetAccountServerData> {
 public:
  GetAccountHandler(const std::string& method_name,
                    qtrade::framework::dao::DbConnectionPoolManager& pool_manager,
                    qtrade::framework::dao::DaoManager& dao_manager)
    : GrpcHandlerInterface(method_name), pool_manager_(pool_manager), dao_manager_(dao_manager) {}
  ~GetAccountHandler() noexcept override = default;

 protected:
  /// 步骤1: 将 gRPC 请求转为业务数据
  Result<GetAccountServerData> ConvertToServerData(::grpc::ServerContext* context,
                                                   const qtrade::account::v1::GetAccountRequest* request) override;

  /// 步骤2: 校验参数合法性
  Result<void> ValidateParams(GetAccountServerData& server_data) override;

  /// 步骤3: 检查前置条件
  Result<void> CheckPreconditions(GetAccountServerData& server_data) override;

  /// 步骤4: 执行业务逻辑（取连接并查 trading_account）
  Result<void> ExecuteBusiness(GetAccountServerData& server_data) override;

  /// 步骤5: 校验操作是否真正生效
  Result<void> VerifyExecutionEffective(GetAccountServerData& server_data) override;

  /// 步骤6: 失败回滚
  void Rollback(GetAccountServerData& server_data) override;

  /// 步骤7: 通知其他服务（失败不回滚）
  Result<void> NotifyService(GetAccountServerData& server_data) override;

  /// 步骤8: 构造响应（password 置空）
  Result<void> BuildResponse(GetAccountServerData& server_data,
                             qtrade::account::v1::GetAccountResponse* response) override;

 private:
  qtrade::framework::dao::DbConnectionPoolManager& pool_manager_;
  qtrade::framework::dao::DaoManager& dao_manager_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_HANDLER_GET_ACCOUNT_HANDLER_HPP_
