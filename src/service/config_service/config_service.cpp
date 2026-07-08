/// @file      config_service.cpp
/// @brief     配置中心支撑服务实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "service/config_service/config_service.hpp"

#include "common/database/database_service_bootstrap.hpp"
#include "common/grpc/grpc_options.hpp"

namespace qtrade::service {

ConfigService::ConfigService() : SupportServiceImpl("qtrade_config_service", 50051) {}

ErrorCode ConfigService::Initialize(const std::string& config_path) {
  std::lock_guard lock(mutex_);

  if (state_ != qtrade::common::support::SupportServiceState::kNew &&
      state_ != qtrade::common::support::SupportServiceState::kTerminated) {
    return ErrorCode::kSystemError;
  }

  state_ = qtrade::common::support::SupportServiceState::kInitializing;
  config_path_ = config_path;
  listen_address_ = qtrade::common::ParseGrpcOptions(config_path, default_port_).ListenAddress();

  const auto context = qtrade::common::BootstrapDatabaseService<IConfigRepository>(
      config_path, CreateConfigRepository, service_name_);
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
