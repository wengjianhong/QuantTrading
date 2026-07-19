/// @file      account_service.hpp
/// @brief     交易账户支撑服务（进程级生命周期控制器）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_SERVICE_HPP_
#define QTRADE_SERVICE_ACCOUNT_SERVICE_HPP_

#include "qtrade/dao/dao_manager.hpp"
#include "qtrade/framework/support/support_sync_service_impl.hpp"
#include "qtrade/service/account_service/grpc/account_grpc_service.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>

namespace qtrade::service {

/// @brief 交易账户支撑服务（同步 gRPC，Unary RPC）
class AccountService final : public qtrade::common::support::SupportSyncServiceImpl<AccountGrpcService> {
 public:
  AccountService();

  ErrorCode Initialize(const std::string& config_path) override;

 protected:
  /// @brief 创建 gRPC Service 并注入 DaoManager
  [[nodiscard]] std::unique_ptr<AccountGrpcService> CreateGrpcService() override;

 private:
  /// 本进程 DaoManager（Initialize 创建，供 Handler 使用）
  std::shared_ptr<qtrade::framework::dao::DaoManager> dao_mgr_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_SERVICE_HPP_
