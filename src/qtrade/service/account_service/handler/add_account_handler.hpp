/// @file      add_account_handler.hpp
/// @brief     AddAccount gRPC 处理器：新增交易账户与加密凭证
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_HANDLER_ADD_ACCOUNT_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_HANDLER_ADD_ACCOUNT_HANDLER_HPP_

#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <qtrade/dao/account_service/trading_account.hpp>
#include <qtrade/dao/dao_manager.hpp>
#include <qtrade/grpc/grpc_handler_interface.hpp>
#include <qtrade/proto/account/v1/account.pb.h>

#include <string>

namespace qtrade::service {

/// @brief AddAccount 管道内业务数据
struct AddAccountServerData {
  /// 明文密码（写入 account_credential）
  std::string password;
  /// 待添加账户元数据
  qtrade::framework::dao::TradingAccountRecord account;
};

/// @brief 添加交易账户（写入 trading_account + account_credential）
class AddAccountHandler final
  : public qtrade::framework::grpc::GrpcHandlerInterface<qtrade::account::v1::AddAccountRequest,
                                                         qtrade::account::v1::AddAccountResponse,
                                                         AddAccountServerData> {
 public:
  AddAccountHandler(const std::string& method_name,
                    qtrade::framework::dao::DbConnectionPoolManager& pool_manager,
                    qtrade::framework::dao::DaoManager& dao_manager)
    : GrpcHandlerInterface(method_name), pool_manager_(pool_manager), dao_manager_(dao_manager) {}
  ~AddAccountHandler() noexcept override = default;

 protected:
  /// 步骤1: 将 gRPC 请求转为业务数据
  Result<AddAccountServerData> ConvertToServerData(::grpc::ServerContext* context,
                                                   const qtrade::account::v1::AddAccountRequest* request) override;

  /// 步骤2: 校验参数合法性
  Result<void> ValidateParams(AddAccountServerData& server_data) override;

  /// 步骤3: 检查前置条件
  Result<void> CheckPreconditions(AddAccountServerData& server_data) override;

  /// 步骤4: 执行业务逻辑（取连接、事务内查重并写入双表）
  Result<void> ExecuteBusiness(AddAccountServerData& server_data) override;

  /// 步骤5: 校验操作是否真正生效
  Result<void> VerifyExecutionEffective(AddAccountServerData& server_data) override;

  /// 步骤6: 失败回滚（DB 事务已在 ExecuteBusiness 内处理）
  void Rollback(AddAccountServerData& server_data) override;

  /// 步骤7: 通知其他服务（失败不回滚）
  Result<void> NotifyService(AddAccountServerData& server_data) override;

  /// 步骤8: 构造响应
  Result<void> BuildResponse(AddAccountServerData& server_data,
                             qtrade::account::v1::AddAccountResponse* response) override;

 private:
  qtrade::framework::dao::DbConnectionPoolManager& pool_manager_;
  qtrade::framework::dao::DaoManager& dao_manager_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_HANDLER_ADD_ACCOUNT_HANDLER_HPP_
