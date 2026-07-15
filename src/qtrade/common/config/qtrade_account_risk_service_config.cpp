/// @file qtrade_account_risk_service_config.cpp
#include "qtrade/common/config/qtrade_account_risk_service_config.hpp"

namespace qtrade::common::config {

std::optional<QtradeAccountRiskServiceConfig> ParseQtradeAccountRiskServiceConfig(const std::string& json) {
  return ParseSupportDatabaseServiceConfig(json);
}

}  // namespace qtrade::common::config
