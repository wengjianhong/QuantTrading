/// @file qtrade_account_risk_service_config.hpp
/// @brief 账户硬风控服务 L0 引导配置。
#ifndef QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/support_database_service_config.hpp"

namespace qtrade::common::config {

using QtradeAccountRiskServiceConfig = SupportDatabaseServiceConfig;

[[nodiscard]] std::optional<QtradeAccountRiskServiceConfig> ParseQtradeAccountRiskServiceConfig(
  const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_
