/// @file      config_service.cpp
/// @brief     配置中心支撑服务实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/config_service.hpp"

#include "qtrade/common/config/qtrade_config_service_config.hpp"
#include "qtrade/common/file/text_file.hpp"
#include "qtrade/dao/config_service/engine/engine_config.hpp"
#include "qtrade/dao/config_service/risk/instance_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/instrument_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/order_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/quote_health_policy.hpp"
#include "qtrade/dao/config_service/risk/strategy_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/tenant_risk_policy.hpp"
#include "qtrade/framework/dao/ddl_utils.hpp"
#include "qtrade/framework/database/database_service_bootstrap.hpp"

namespace qtrade::service {

namespace {

/// @brief 确保 config-service 业务配置与 A 段风险策略表存在
/// @param connection 数据库连接；不可为 nullptr
/// @return ErrorCode::kSuccess 表示成功；任一表 DDL 失败返回 ErrorCode::kSystemError
ErrorCode EnsureConfigSchema(cpputils::database::IConnection* connection) {
  using qtrade::framework::dao::EnsureTableSchema;
  if (EnsureTableSchema(connection, qtrade::framework::dao::EngineConfig::Instance()) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (EnsureTableSchema(connection, qtrade::framework::dao::TenantRiskPolicy::Instance()) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (EnsureTableSchema(connection, qtrade::framework::dao::InstanceRiskPolicy::Instance()) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (EnsureTableSchema(connection, qtrade::framework::dao::StrategyRiskPolicy::Instance()) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (EnsureTableSchema(connection, qtrade::framework::dao::InstrumentRiskPolicy::Instance()) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (EnsureTableSchema(connection, qtrade::framework::dao::OrderRiskPolicy::Instance()) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (EnsureTableSchema(connection, qtrade::framework::dao::QuoteHealthPolicy::Instance()) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
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
