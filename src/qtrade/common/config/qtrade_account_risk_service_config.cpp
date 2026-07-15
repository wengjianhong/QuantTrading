/// @file      qtrade_account_risk_service_config.cpp
/// @brief     QtradeAccountRiskServiceConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_account_risk_service_config.hpp"

namespace qtrade::common::config {

std::optional<QtradeAccountRiskServiceConfig> ParseQtradeAccountRiskServiceConfig(const std::string& json) {
  return ParseSupportDatabaseServiceConfig(json);
}

}  // namespace qtrade::common::config
