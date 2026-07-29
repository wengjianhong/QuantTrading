/// @file      qtrade_account_risk_service_bootstrap_config.hpp
/// @brief     qtrade_account_risk_service.json 配置结构
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_BOOTSTRAP_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_BOOTSTRAP_CONFIG_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/common/config/service_config.hpp"

#include <optional>

namespace qtrade::common::config {

/// @brief 预占运行参数
struct AccountRiskReservationConfig {
  /// 默认预占 TTL（毫秒）
  int default_ttl_ms = 5000;
  /// 过期扫描间隔（毫秒）
  int expire_scan_interval_ms = 500;
};

/// @brief 对应 config/qtrade_account_risk_service.json
struct QtradeAccountRiskServiceBootstrapConfig {
  /// gRPC 监听（JSON 键仍为 grpc）
  ServiceConfig grpc;
  /// 数据库
  DatabaseConfig database;
  /// 预占参数
  AccountRiskReservationConfig reservation;
};

/// @brief 从账户硬风控服务配置 JSON 对象解析
/// @param config_node 形如 { "grpc", "database", "reservation" } 的对象
/// @return 解析结果；非对象或缺必填段时返回 nullopt
[[nodiscard]] std::optional<QtradeAccountRiskServiceBootstrapConfig> ParseQtradeAccountRiskServiceBootstrapConfig(
  const nlohmann::json& config_node);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_BOOTSTRAP_CONFIG_HPP_
