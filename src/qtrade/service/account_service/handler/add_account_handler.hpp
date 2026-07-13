/// @file      add_account_handler.hpp
/// @brief     AddAccount gRPC 处理器：新增交易账户与加密凭证
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_HANDLER_ADD_ACCOUNT_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_HANDLER_ADD_ACCOUNT_HANDLER_HPP_

#include <qtrade/proto/account/v1/account.pb.h>
#include <qtrade_framework/dao/trading_account.hpp>
#include <qtrade_framework/grpc/grpc_handler_interface.hpp>

#include <string>

namespace qtrade::service {

/// @brief AddAccount 管道内业务数据
struct AddAccountServerData {
  bool account_inserted = false;                         ///< trading_account 是否已写入（用于回滚）
  std::string password;                                  ///< 明文密码（写入 account_credential）
  qtrade::framework::dao::TradingAccountRecord account;  ///< 待添加账户元数据
};

/// @brief 添加交易账户（写入 trading_account + account_credential）
class AddAccountHandler final
  : public qtrade::framework::grpc::GrpcHandlerInterface<qtrade::account::v1::AddAccountRequest,
                                                         qtrade::account::v1::AddAccountResponse,
                                                         AddAccountServerData> {
 public:
  explicit AddAccountHandler(const std::string& method_name) : GrpcHandlerInterface(method_name) {}
  ~AddAccountHandler() noexcept override = default;

 protected:
  /// 步骤1: 将 gRPC 请求转为业务数据
  Result<AddAccountServerData> ConvertToServerData(::grpc::ServerContext* context,
                                                   const qtrade::account::v1::AddAccountRequest* request) override;

  /// 步骤2: 校验参数合法性
  Result<void> ValidateParams(AddAccountServerData& server_data) override;

  /// 步骤3: 检查前置条件（账户是否已存在）
  Result<void> CheckPreconditions(AddAccountServerData& server_data) override;

  /// 步骤4: 执行业务逻辑（加密密码、写入双表）
  Result<void> ExecuteBusiness(AddAccountServerData& server_data) override;

  /// 步骤5: 校验操作是否真正生效
  Result<void> VerifyExecutionEffective(AddAccountServerData& server_data) override;

  /// 步骤6: 失败回滚（删除已插入的 trading_account）
  void Rollback(AddAccountServerData& server_data) override;

  /// 步骤7: 通知其他服务（失败不回滚）
  Result<void> NotifyService(AddAccountServerData& server_data) override;

  /// 步骤8: 构造响应
  Result<void> BuildResponse(AddAccountServerData& server_data,
                             qtrade::account::v1::AddAccountResponse* response) override;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_HANDLER_ADD_ACCOUNT_HANDLER_HPP_
