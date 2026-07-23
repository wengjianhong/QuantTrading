/// @file      update_account_handler.hpp
/// @brief     UpdateAccount gRPC 处理器：更新交易账户与可选凭证
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_HANDLER_UPDATE_ACCOUNT_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_HANDLER_UPDATE_ACCOUNT_HANDLER_HPP_

#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <qtrade/dao/account_service/trading_account.hpp>
#include <qtrade/dao/dao_manager.hpp>
#include <qtrade/grpc/grpc_handler_interface.hpp>
#include <qtrade/proto/account/v1/account.pb.h>

#include <string>

namespace qtrade::service {

/// @brief UpdateAccount 管道内业务数据
struct UpdateAccountServerData {
  /// 是否同步更新密码
  bool update_password = false;
  /// 新明文密码（空表示不更新凭证）
  std::string password;
  /// 待更新账户元数据
  qtrade::framework::dao::TradingAccountRecord account;
};

/// @brief 更新交易账户（可选更新 account_credential）
class UpdateAccountHandler final
  : public qtrade::framework::grpc::GrpcHandlerInterface<qtrade::account::v1::UpdateAccountRequest,
                                                         qtrade::account::v1::UpdateAccountResponse,
                                                         UpdateAccountServerData> {
 public:
  UpdateAccountHandler(const std::string& method_name,
                       qtrade::framework::dao::DbConnectionPoolManager& pool_manager,
                       qtrade::framework::dao::DaoManager& dao_manager)
    : GrpcHandlerInterface(method_name), pool_manager_(pool_manager), dao_manager_(dao_manager) {}
  ~UpdateAccountHandler() noexcept override = default;

 protected:
  /// 步骤1: 将 gRPC 请求转为业务数据
  Result<UpdateAccountServerData> ConvertToServerData(
    ::grpc::ServerContext* context, const qtrade::account::v1::UpdateAccountRequest* request) override;

  /// 步骤2: 校验参数合法性
  Result<void> ValidateParams(UpdateAccountServerData& server_data) override;

  /// 步骤3: 检查前置条件
  Result<void> CheckPreconditions(UpdateAccountServerData& server_data) override;

  /// 步骤4: 执行业务逻辑（取连接、事务内更新 trading_account，可选更新 credential）
  Result<void> ExecuteBusiness(UpdateAccountServerData& server_data) override;

  /// 步骤5: 校验操作是否真正生效
  Result<void> VerifyExecutionEffective(UpdateAccountServerData& server_data) override;

  /// 步骤6: 失败回滚（DB 事务已在 ExecuteBusiness 内处理）
  void Rollback(UpdateAccountServerData& server_data) override;

  /// 步骤7: 通知其他服务（失败不回滚）
  Result<void> NotifyService(UpdateAccountServerData& server_data) override;

  /// 步骤8: 构造响应
  Result<void> BuildResponse(UpdateAccountServerData& server_data,
                             qtrade::account::v1::UpdateAccountResponse* response) override;

 private:
  qtrade::framework::dao::DbConnectionPoolManager& pool_manager_;
  qtrade::framework::dao::DaoManager& dao_manager_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_HANDLER_UPDATE_ACCOUNT_HANDLER_HPP_
