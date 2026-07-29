/// @file      qtrade_account_risk_service_bootstrap_config.cpp
/// @brief     QtradeAccountRiskServiceBootstrapConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_account_risk_service_bootstrap_config.hpp"

#include "spdlog/spdlog.h"

namespace qtrade::common::config {

std::optional<QtradeAccountRiskServiceBootstrapConfig> ParseQtradeAccountRiskServiceBootstrapConfig(
  const nlohmann::json& config_node) {
  if (!config_node.is_object()) {
    return std::nullopt;
  }
  if (!config_node.contains("grpc") || !config_node.at("grpc").is_object()) {
    spdlog::error("grpc config missing or not an object");
    return std::nullopt;
  }
  const auto grpc = ParseServiceEndpoint(config_node.at("grpc"));
  if (!grpc.has_value()) {
    return std::nullopt;
  }

  QtradeAccountRiskServiceBootstrapConfig config;
  config.grpc = grpc.value();
  if (config_node.contains("database") && config_node.at("database").is_object()) {
    config.database = ParseDatabaseConfigFromSection(config_node.at("database"));
  }

  if (config_node.contains("reservation") && config_node.at("reservation").is_object()) {
    const auto& reservation = config_node.at("reservation");
    config.reservation.default_ttl_ms = reservation.value("default_ttl_ms", config.reservation.default_ttl_ms);
    config.reservation.expire_scan_interval_ms =
      reservation.value("expire_scan_interval_ms", config.reservation.expire_scan_interval_ms);
  }
  if (config.reservation.default_ttl_ms <= 0 || config.reservation.expire_scan_interval_ms <= 0) {
    spdlog::error("reservation.default_ttl_ms/expire_scan_interval_ms invalid");
    return std::nullopt;
  }
  return config;
}

}  // namespace qtrade::common::config
