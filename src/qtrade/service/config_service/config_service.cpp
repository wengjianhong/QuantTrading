/// @file      config_service.cpp
/// @brief     配置中心支撑服务实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/config_service.hpp"

#include "qtrade/common/config/qtrade_config_service_config.hpp"
#include "qtrade/common/file/text_file.hpp"
#include "qtrade/dao/engine_config.hpp"
#include "qtrade/framework/dao/ddl_utils.hpp"
#include "qtrade/framework/database/database_service_bootstrap.hpp"

namespace qtrade::service {

namespace {

ErrorCode EnsureConfigSchema(cpputils::database::IConnection* connection) {
  return qtrade::framework::dao::EnsureTableSchema(connection, qtrade::framework::dao::EngineConfig::Instance());
}

}  // namespace

ConfigService::ConfigService() : SupportAsyncServiceImpl("qtrade_config_service", 50051) {}

ErrorCode ConfigService::Initialize(const std::string& config_path) {
  std::lock_guard lock(mutex_);

  if (state_ != qtrade::common::support::SupportServiceState::kNew &&
      state_ != qtrade::common::support::SupportServiceState::kTerminated) {
    return ErrorCode::kSystemError;
  }

  state_ = qtrade::common::support::SupportServiceState::kInitializing;
  config_path_ = config_path;

  const auto json_text = qtrade::common::ReadTextFile(config_path);
  if (!json_text.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kNotFound;
    return last_error_;
  }
  const auto config = qtrade::common::config::ParseQtradeConfigServiceConfig(*json_text);
  if (!config.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kNotFound;
    return last_error_;
  }
  listen_address_ = config->grpc.ListenAddress();

  const auto context = qtrade::common::BootstrapDatabaseConnection(config->database, EnsureConfigSchema, service_name_);
  if (!context.connection) {
    connection_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternal;
    return last_error_;
  }

  connection_ = std::move(context.connection);
  last_error_ = ErrorCode::kSuccess;
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::service
