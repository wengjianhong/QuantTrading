/// @file      account_service.cpp
/// @brief     交易账户支撑服务实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/account_service.hpp"

#include "qtrade/dao/account_credential.hpp"
#include "qtrade/dao/trading_account.hpp"
#include "qtrade/framework/dao/ddl_utils.hpp"
#include "qtrade/framework/database/database_service_bootstrap.hpp"
#include "qtrade/framework/grpc/grpc_options.hpp"

namespace qtrade::service {

namespace {

ErrorCode EnsureAccountSchema(cpputils::database::IConnection* connection) {
  if (connection == nullptr) {
    return ErrorCode::kSystemError;
  }
  if (const auto rc =
        qtrade::framework::dao::EnsureTableSchema(connection, qtrade::framework::dao::TradingAccount::Instance());
      rc != ErrorCode::kSuccess) {
    return rc;
  }
  return qtrade::framework::dao::EnsureTableSchema(connection, qtrade::framework::dao::AccountCredential::Instance());
}

}  // namespace

AccountService::AccountService() : SupportSyncServiceImpl("qtrade_account_service", 50052) {}

ErrorCode AccountService::Initialize(const std::string& config_path) {
  std::lock_guard lock(mutex_);

  if (state_ != qtrade::common::support::SupportServiceState::kNew &&
      state_ != qtrade::common::support::SupportServiceState::kTerminated) {
    return ErrorCode::kSystemError;
  }

  state_ = qtrade::common::support::SupportServiceState::kInitializing;
  config_path_ = config_path;
  listen_address_ = qtrade::common::ParseGrpcOptions(config_path, default_port_).ListenAddress();

  const auto context = qtrade::common::BootstrapDatabaseConnection(config_path, EnsureAccountSchema, service_name_);
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
