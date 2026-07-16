/// @file      qtrade_account_risk_service_config.cpp
/// @brief     QtradeAccountRiskServiceConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_account_risk_service_config.hpp"

#include "qtrade/common/json/json_util.hpp"
#include "spdlog/spdlog.h"

namespace qtrade::common::config {

std::optional<QtradeAccountRiskServiceConfig> ParseQtradeAccountRiskServiceConfig(const std::string& json) {
  const auto root = ParseJsonString(json);
  if (!root.has_value()) {
    return std::nullopt;
  }
  const auto& root_json = root.value();
  const auto grpc = ParseServiceConfig(root_json);
  if (!grpc.has_value()) {
    return std::nullopt;
  }

  QtradeAccountRiskServiceConfig config;
  config.grpc = grpc.value();
  config.database = ParseDatabaseConfigFromRoot(root_json);

  if (root_json.contains("reservation") && root_json.at("reservation").is_object()) {
    const auto& reservation = root_json.at("reservation");
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
