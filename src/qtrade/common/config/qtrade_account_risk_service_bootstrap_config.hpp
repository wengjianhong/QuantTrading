/// @file      qtrade_account_risk_service_bootstrap_config.hpp
/// @brief     qtrade_account_risk_service.json 配置结构
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_BOOTSTRAP_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_BOOTSTRAP_CONFIG_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/common/config/service_config.hpp"
#include "qtrade/service/account_risk_service/account_risk_service_define.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 账户硬风控服务本地运行参数（JSON：config）
struct AccountRiskServiceProcessConfig {
  /// 日志目录
  std::string log_dir = qtrade::service::account_risk_service::kDefaultLogDir;
  /// 日志文件名
  std::string log_filename = qtrade::service::account_risk_service::kDefaultLogFilename;
  /// 默认预占 TTL（毫秒）
  int default_ttl_ms = 5000;
  /// 过期扫描间隔（毫秒）
  int expire_scan_interval_ms = 500;
};

/// @brief 对应 config/qtrade_account_risk_service.json
struct QtradeAccountRiskServiceBootstrapConfig {
  /// 服务本地配置
  AccountRiskServiceProcessConfig config;
  /// gRPC 监听
  ServiceConfig grpc;
  /// 数据库
  DatabaseConfig database;
};

/// @brief 从账户硬风控服务配置 JSON 对象解析
/// @param config_node 形如 { "config", "grpc", "database" } 的对象
/// @return 解析结果；非对象或缺必填段时返回 nullopt
[[nodiscard]] std::optional<QtradeAccountRiskServiceBootstrapConfig> ParseQtradeAccountRiskServiceBootstrapConfig(
  const nlohmann::json& config_node);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ACCOUNT_RISK_SERVICE_BOOTSTRAP_CONFIG_HPP_
