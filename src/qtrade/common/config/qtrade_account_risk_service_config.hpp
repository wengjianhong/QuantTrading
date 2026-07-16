/// @file      qtrade_account_risk_service_config.hpp
/// @brief     qtrade_account_risk_service.json 配置结构
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/common/config/service_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 预占运行参数
struct AccountRiskReservationConfig {
  /// 默认预占 TTL（毫秒）
  int default_ttl_ms = 5000;
  /// 过期扫描间隔（毫秒）
  int expire_scan_interval_ms = 500;
};

/// @brief 对应 config/qtrade_account_risk_service.json
struct QtradeAccountRiskServiceConfig {
  /// gRPC 监听（JSON 键仍为 grpc）
  ServiceConfig grpc;
  /// 数据库
  DatabaseConfig database;
  /// 预占参数
  AccountRiskReservationConfig reservation;
};

/// @brief 从 JSON 字符串解析账户硬风控服务配置
/// @param json JSON 文本
/// @return 解析结果；非法或缺必填段时返回 nullopt
[[nodiscard]] std::optional<QtradeAccountRiskServiceConfig> ParseQtradeAccountRiskServiceConfig(
  const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_CONFIG_HPP_
