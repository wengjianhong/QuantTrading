/// @file      account_service.cpp
/// @brief     交易账户支撑服务实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/account_service.hpp"

#include "qtrade/common/config/qtrade_account_service_config.hpp"
#include "qtrade/common/json/json_util.hpp"
#include "qtrade/framework/dao/ddl_utils.hpp"
#include "qtrade/framework/database/db_connection_pool_manager.hpp"

namespace qtrade::service {

AccountService::AccountService() : SupportSyncServiceImpl("qtrade_account_service", 50052) {}

ErrorCode AccountService::Initialize(const std::string& config_path) {
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
  const auto config = qtrade::common::config::ParseQtradeAccountServiceConfig(*config_node);
  if (!config.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kNotFound;
    return last_error_;
  }
  listen_address_ = config->grpc.Address();

  // 1. 创建数据库连接池；2. 创建 DaoManager 并确保全部表结构
  connection_ = std::make_shared<qtrade::framework::dao::DbConnectionPoolManager>(config->database.pool);
  if (!connection_->IsReady()) {
    connection_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternal;
    return last_error_;
  }

  dao_ = std::make_shared<qtrade::framework::dao::DaoManager>();
  auto schema_connection = connection_->Acquire();
  if (schema_connection == nullptr) {
    dao_.reset();
    connection_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternal;
    return last_error_;
  }
  auto* database = schema_connection.get();
  if (qtrade::framework::dao::EnsureTableSchema(
        database, dao_->Get<qtrade::framework::dao::TradingAccount>()) != ErrorCode::kSuccess ||
      qtrade::framework::dao::EnsureTableSchema(
        database, dao_->Get<qtrade::framework::dao::AccountCredential>()) != ErrorCode::kSuccess) {
    dao_.reset();
    connection_.reset();
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    last_error_ = ErrorCode::kInternal;
    return last_error_;
  }

  last_error_ = ErrorCode::kSuccess;
  return ErrorCode::kSuccess;
}

std::unique_ptr<AccountGrpcService> AccountService::CreateGrpcService() {
  if (!dao_) {
    return nullptr;
  }
  return std::make_unique<AccountGrpcService>(connection_, dao_);
}

}  // namespace qtrade::service
