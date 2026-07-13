/// @file      get_credential_handler.hpp
/// @brief     GetCredential gRPC 处理器：引擎按需拉取登录凭证
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_HANDLER_GET_CREDENTIAL_HANDLER_HPP_
#define QTRADE_SERVICE_ACCOUNT_HANDLER_GET_CREDENTIAL_HANDLER_HPP_

#include <qtrade/proto/account/v1/account.pb.h>
#include <qtrade_framework/dao/trading_account.hpp>
#include <qtrade_framework/grpc/grpc_handler_interface.hpp>

#include <string>

namespace qtrade::service {

/// @brief GetCredential 管道内业务数据
struct GetCredentialServerData {
  std::string tenant_id;                                 ///< 租户 ID
  std::string account_id;                                ///< 交易账户号
  std::string engine_id;                                 ///< 引擎实例 ID（用于审计日志）
  std::string password;                                  ///< 解密后的明文密码
  qtrade::framework::dao::TradingAccountRecord account;  ///< 账户元数据
};

/// @brief 获取登录凭证（含明文密码，供引擎冷启动/换密使用）
class GetCredentialHandler final
  : public qtrade::framework::grpc::GrpcHandlerInterface<qtrade::account::v1::GetCredentialRequest,
                                                         qtrade::account::v1::GetCredentialResponse,
                                                         GetCredentialServerData> {
 public:
  explicit GetCredentialHandler(const std::string& method_name) : GrpcHandlerInterface(method_name) {}
  ~GetCredentialHandler() noexcept override = default;

 protected:
  /// 步骤1: 将 gRPC 请求转为业务数据
  Result<GetCredentialServerData> ConvertToServerData(
    ::grpc::ServerContext* context, const qtrade::account::v1::GetCredentialRequest* request) override;

  /// 步骤2: 校验参数合法性
  Result<void> ValidateParams(GetCredentialServerData& server_data) override;

  /// 步骤3: 检查前置条件
  Result<void> CheckPreconditions(GetCredentialServerData& server_data) override;

  /// 步骤4: 执行业务逻辑（查账户、解密凭证）
  Result<void> ExecuteBusiness(GetCredentialServerData& server_data) override;

  /// 步骤5: 校验操作是否真正生效
  Result<void> VerifyExecutionEffective(GetCredentialServerData& server_data) override;

  /// 步骤6: 失败回滚
  void Rollback(GetCredentialServerData& server_data) override;

  /// 步骤7: 通知其他服务（审计日志）
  Result<void> NotifyService(GetCredentialServerData& server_data) override;

  /// 步骤8: 构造响应
  Result<void> BuildResponse(GetCredentialServerData& server_data,
                             qtrade::account::v1::GetCredentialResponse* response) override;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_HANDLER_GET_CREDENTIAL_HANDLER_HPP_
