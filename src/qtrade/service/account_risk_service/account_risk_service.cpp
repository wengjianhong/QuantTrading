/// @file      account_risk_service.cpp
/// @brief     账户硬风控服务生命周期实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_risk_service/account_risk_service.hpp"

#include "qtrade/common/config/qtrade_account_risk_service_config.hpp"
#include "qtrade/common/json/json_util.hpp"
#include "qtrade/framework/dao/ddl_utils.hpp"
#include "qtrade/framework/database/db_connection_pool_manager.hpp"

namespace qtrade::service {

AccountRiskService::AccountRiskService() : SupportSyncServiceImpl("qtrade_account_risk_service", 50060) {}

ErrorCode AccountRiskService::Initialize(const std::string& config_path) {
  // 1. 校验生命周期并读取配置文本
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
    return last_error_ = ErrorCode::kNotFound;
  }

  // 2. 解析 L0 配置并解析监听地址
  const auto config = qtrade::common::config::ParseQtradeAccountRiskServiceConfig(*config_node);
  if (!config.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kInternal;
  }
  listen_address_ = config->grpc.Address();

  // 3. 创建数据库连接池、DaoManager 并确保表结构
  connection_ = std::make_shared<qtrade::framework::dao::DbConnectionPoolManager>(config->database.pool);
  if (!connection_->IsReady()) {
    connection_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kInternal;
  }

  dao_ = std::make_shared<qtrade::framework::dao::DaoManager>();
  auto schema_connection = connection_->Acquire();
  if (schema_connection == nullptr) {
    dao_.reset();
    connection_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kInternal;
  }
  auto* database = schema_connection.get();
  if (qtrade::framework::dao::EnsureTableSchema(
        database, dao_->Get<qtrade::framework::dao::AccountRiskPolicy>()) != ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(
        database, dao_->Get<qtrade::framework::dao::OrderReservation>()) != ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(
        database, dao_->Get<qtrade::framework::dao::AccountRiskLedger>()) != ErrorCode::kSuccess) {
    dao_.reset();
    connection_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kInternal;
  }
  return last_error_ = ErrorCode::kSuccess;
}

std::unique_ptr<AccountRiskGrpcService> AccountRiskService::CreateGrpcService() {
  if (!dao_) {
    return nullptr;
  }
  return std::make_unique<AccountRiskGrpcService>(connection_, dao_);
}

}  // namespace qtrade::service
