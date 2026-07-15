/// @file      qtrade_account_risk_service_config.hpp
/// @brief     qtrade_account_risk_service.json 配置结构
/// @details   账户硬风控服务 L0 引导配置；语义与 SupportDatabaseServiceConfig 相同
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/support_database_service_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 对应 config/qtrade_account_risk_service.json
using QtradeAccountRiskServiceConfig = SupportDatabaseServiceConfig;

/// @brief 从 JSON 字符串解析账户硬风控服务配置
/// @param json JSON 文本
/// @return 解析结果；JSON 非法或缺少 grpc 段时返回 nullopt
[[nodiscard]] std::optional<QtradeAccountRiskServiceConfig> ParseQtradeAccountRiskServiceConfig(
  const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_
