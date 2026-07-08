/// @file      account_service.cpp
/// @brief     交易账户支撑服务实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "service/account_service/account_service.hpp"

#include "common/database/database_service_bootstrap.hpp"
#include "common/grpc/grpc_options.hpp"

namespace qtrade::service {

AccountService::AccountService() : SupportServiceImpl("qtrade_account_service", 50052) {}

ErrorCode AccountService::Initialize(const std::string& config_path) {
  std::lock_guard lock(mutex_);

  if (state_ != qtrade::common::support::SupportServiceState::kNew &&
      state_ != qtrade::common::support::SupportServiceState::kTerminated) {
    return ErrorCode::kSystemError;
  }

  state_ = qtrade::common::support::SupportServiceState::kInitializing;
  config_path_ = config_path;
  listen_address_ = qtrade::common::ParseGrpcOptions(config_path, default_port_).ListenAddress();

  const auto context =
    qtrade::common::BootstrapDatabaseService<IAccountRepository>(config_path, CreateAccountRepository, service_name_);
  if (!context.repository) {
    repository_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternal;
    return last_error_;
  }

  repository_ = std::move(context.repository);
  last_error_ = ErrorCode::kSuccess;
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::service
