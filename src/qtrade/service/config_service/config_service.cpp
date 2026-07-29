/// @file      config_service.cpp
/// @brief     配置中心支撑服务实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/config_service.hpp"

#include "qtrade/common/config/qtrade_config_service_bootstrap_config.hpp"
#include "qtrade/common/json/json_util.hpp"
#include "qtrade/dao/dao_define.hpp"
#include "qtrade/framework/dao/ddl_utils.hpp"
#include "qtrade/framework/database/db_connection_pool_manager.hpp"

namespace qtrade::service {

ConfigService::ConfigService() : SupportAsyncServiceImpl("qtrade_config_service", 50051) {}

ErrorCode ConfigService::Initialize(const std::string& config_path) {
  std::lock_guard lock(mutex_);

  if (state_ != qtrade::common::support::SupportServiceState::kNew &&
      state_ != qtrade::common::support::SupportServiceState::kTerminated) {
    return ErrorCode::kSystemError;
  }

  state_ = qtrade::common::support::SupportServiceState::kInitializing;
  config_path_ = config_path;

  const auto config_node = qtrade::common::LoadJsonFile(config_path);
  if (!config_node.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kNotFound;
    return last_error_;
  }
  const auto config = qtrade::common::config::ParseQtradeConfigServiceBootstrapConfig(*config_node);
  if (!config.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kNotFound;
    return last_error_;
  }
  listen_address_ = config->grpc.Address();

  // 1. 创建数据库连接池；2. 创建 DaoManager 并确保全部表结构
  connection_pool_mgr_ = std::make_shared<qtrade::framework::dao::DbConnectionPoolManager>();
  if (!connection_pool_mgr_->AddConnectionPool(qtrade::framework::dao::kConfigDatabaseName, config->database.pool) ||
      !connection_pool_mgr_->IsReady()) {
    connection_pool_mgr_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternalError;
    return last_error_;
  }

  dao_mgr_ = std::make_shared<qtrade::framework::dao::DaoManager>();
  auto schema_connection = connection_pool_mgr_->Acquire(qtrade::framework::dao::kConfigDatabaseName);
  if (schema_connection == nullptr) {
    dao_mgr_.reset();
    connection_pool_mgr_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternalError;
    return last_error_;
  }
  auto* database = schema_connection.get();
  if (qtrade::framework::dao::EnsureTableSchema(database, dao_mgr_->Get<qtrade::framework::dao::EngineConfig>()) !=
        ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(database, dao_mgr_->Get<qtrade::framework::dao::TenantRiskPolicy>()) !=
        ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(
        database, dao_mgr_->Get<qtrade::framework::dao::InstanceRiskPolicy>()) != ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(
        database, dao_mgr_->Get<qtrade::framework::dao::StrategyRiskPolicy>()) != ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(
        database, dao_mgr_->Get<qtrade::framework::dao::InstrumentRiskPolicy>()) != ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(database, dao_mgr_->Get<qtrade::framework::dao::OrderRiskPolicy>()) !=
        ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(database, dao_mgr_->Get<qtrade::framework::dao::QuoteHealthPolicy>()) !=
        ErrorCode::kSuccess) {
    dao_mgr_.reset();
    connection_pool_mgr_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternalError;
    return last_error_;
  }

  last_error_ = ErrorCode::kSuccess;
  return ErrorCode::kSuccess;
}

void ConfigService::InitHandler() {
  handler_->Init(&async_service_, grpc_server_->CompletionQueue(), connection_pool_mgr_, dao_mgr_);
}

}  // namespace qtrade::service
