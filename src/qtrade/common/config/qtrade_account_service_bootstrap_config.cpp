/// @file      qtrade_account_service_bootstrap_config.cpp
/// @brief     QtradeAccountServiceBootstrapConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_account_service_bootstrap_config.hpp"

#include "spdlog/spdlog.h"

namespace qtrade::common::config {

std::optional<QtradeAccountServiceBootstrapConfig> ParseQtradeAccountServiceBootstrapConfig(
  const nlohmann::json& config_node) {
  if (!config_node.is_object()) {
    spdlog::error("account service config must be an object");
    return std::nullopt;
  }
  if (!config_node.contains("config") || !config_node.at("config").is_object()) {
    spdlog::error("config missing or not an object");
    return std::nullopt;
  }
  if (!config_node.contains("grpc") || !config_node.at("grpc").is_object()) {
    spdlog::error("grpc config missing or not an object");
    return std::nullopt;
  }
  const auto grpc = ParseServiceEndpoint(config_node.at("grpc"));
  if (!grpc.has_value()) {
    spdlog::error("parse grpc config failed");
    return std::nullopt;
  }

  QtradeAccountServiceBootstrapConfig out;
  const auto& process = config_node.at("config");
  out.config.log_dir = process.value("log_dir", out.config.log_dir);
  out.config.log_filename = process.value("log_filename", out.config.log_filename);
  if (out.config.log_dir.empty() || out.config.log_filename.empty()) {
    spdlog::error("config.log_dir/log_filename required");
    return std::nullopt;
  }

  out.grpc = grpc.value();
  if (config_node.contains("database") && config_node.at("database").is_object()) {
    out.database = ParseDatabaseConfigFromSection(config_node.at("database"));
  }
  return out;
}

}  // namespace qtrade::common::config
