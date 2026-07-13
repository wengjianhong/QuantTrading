/// @file      account_service.hpp
/// @brief     交易账户支撑服务（进程级生命周期控制器）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_SERVICE_HPP_
#define QTRADE_SERVICE_ACCOUNT_SERVICE_HPP_

#include "qtrade/service/account_service/grpc/account_grpc_service.hpp"
#include "qtrade/framework/support/support_sync_service_impl.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <string>

namespace qtrade::service {

/// @brief 交易账户支撑服务（同步 gRPC，Unary RPC）
class AccountService final : public qtrade::common::support::SupportSyncServiceImpl<AccountGrpcService> {
 public:
  AccountService();

  ErrorCode Initialize(const std::string& config_path) override;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_SERVICE_HPP_
