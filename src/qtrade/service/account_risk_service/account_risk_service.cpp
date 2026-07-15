/// @file account_risk_service.cpp
/// @brief 账户硬风控服务生命周期实现
#include "qtrade/service/account_risk_service/account_risk_service.hpp"

#include "qtrade/common/config/qtrade_account_risk_service_config.hpp"
#include "qtrade/common/file/text_file.hpp"
#include "qtrade/framework/database/database_service_bootstrap.hpp"

namespace qtrade::service {
namespace {

ErrorCode EnsureAccountRiskSchema(cpputils::database::IConnection* connection) {
  if (connection == nullptr) {
    return ErrorCode::kSystemError;
  }
  constexpr const char* kPolicySql =
    "CREATE TABLE IF NOT EXISTS account_risk_policy ("
    "tenant_id TEXT NOT NULL, account_id TEXT NOT NULL, version BIGINT NOT NULL, "
    "valid_until_unix_ms BIGINT NOT NULL, max_notional DOUBLE NOT NULL, max_margin DOUBLE NOT NULL, "
    "max_gross_exposure DOUBLE NOT NULL, max_open_orders BIGINT NOT NULL, safety_buffer DOUBLE NOT NULL, "
    "enabled BOOLEAN NOT NULL, PRIMARY KEY (tenant_id, account_id));";
  constexpr const char* kReservationSql =
    "CREATE TABLE IF NOT EXISTS order_reservation ("
    "tenant_id TEXT NOT NULL, account_id TEXT NOT NULL, order_id TEXT NOT NULL, reservation_id TEXT NOT NULL, "
    "status TEXT NOT NULL, reserved_notional DOUBLE NOT NULL, reserved_margin DOUBLE NOT NULL, "
    "expires_at_unix_ms BIGINT NOT NULL, PRIMARY KEY (tenant_id, account_id, order_id));";
  if (!connection->Execute(kPolicySql) || !connection->Execute(kReservationSql)) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

}  // namespace

AccountRiskService::AccountRiskService() : SupportSyncServiceImpl("qtrade_account_risk_service", 50060) {}

ErrorCode AccountRiskService::Initialize(const std::string& config_path) {
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
    return last_error_ = ErrorCode::kNotFound;
  }
  const auto config = qtrade::common::config::ParseQtradeAccountRiskServiceConfig(*json_text);
  if (!config.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kInternal;
  }
  listen_address_ = config->grpc.ListenAddress();
  const auto context =
    qtrade::common::BootstrapDatabaseConnection(config->database, EnsureAccountRiskSchema, service_name_);
  if (!context.connection) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kInternal;
  }
  connection_ = std::move(context.connection);
  return last_error_ = ErrorCode::kSuccess;
}

}  // namespace qtrade::service
