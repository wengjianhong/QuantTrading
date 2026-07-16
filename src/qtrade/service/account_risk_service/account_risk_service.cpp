/// @file      account_risk_service.cpp
/// @brief     账户硬风控服务生命周期实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_risk_service/account_risk_service.hpp"

#include "qtrade/common/config/qtrade_account_risk_service_config.hpp"
#include "qtrade/common/file/text_file.hpp"
#include "qtrade/dao/account_risk_service/account_risk_ledger.hpp"
#include "qtrade/dao/account_risk_service/account_risk_policy.hpp"
#include "qtrade/dao/account_risk_service/order_reservation.hpp"
#include "qtrade/framework/dao/ddl_utils.hpp"
#include "qtrade/framework/database/database_service_bootstrap.hpp"

namespace qtrade::service {
namespace {

/// @brief 确保 E 段账户风控相关表存在
/// @param connection 数据库连接；不可为 nullptr
/// @return ErrorCode::kSuccess 表示成功；连接为空或 DDL 失败返回 ErrorCode::kSystemError
ErrorCode EnsureAccountRiskSchema(cpputils::database::IConnection* connection) {
  if (connection == nullptr) {
    return ErrorCode::kSystemError;
  }
  if (qtrade::framework::dao::EnsureTableSchema(connection, qtrade::framework::dao::AccountRiskPolicy::Instance()) !=
      ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (qtrade::framework::dao::EnsureTableSchema(connection, qtrade::framework::dao::OrderReservation::Instance()) !=
      ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  if (qtrade::framework::dao::EnsureTableSchema(connection, qtrade::framework::dao::AccountRiskLedger::Instance()) !=
      ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

}  // namespace

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

  const auto json_text = qtrade::common::ReadTextFile(config_path);
  if (!json_text.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kNotFound;
  }

  // 2. 解析 L0 配置并解析监听地址
  const auto config = qtrade::common::config::ParseQtradeAccountRiskServiceConfig(*json_text);
  if (!config.has_value()) {
    state_ = qtrade::common::support::SupportServiceState::kFailed;
    return last_error_ = ErrorCode::kInternal;
  }
  listen_address_ = config->grpc.ListenAddress();

  // 3. 引导数据库连接并确保表结构
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
