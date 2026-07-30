/// @file      qtrade_engine_bootstrap_config.cpp
/// @brief     QtradeEngineBootstrapConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"

#include "spdlog/spdlog.h"

namespace qtrade::common::config {
namespace {

/// @brief 在 support_services 数组中按 name 查找并解析端点
[[nodiscard]] std::optional<ServiceConfig> ParseSupportServiceByName(const nlohmann::json& support_services,
                                                                     const char* name) {
  if (!support_services.is_array()) {
    spdlog::error("support_services must be an array");
    return std::nullopt;
  }
  for (const auto& item : support_services) {
    if (!item.is_object()) {
      continue;
    }
    if (item.value("name", "") != name) {
      continue;
    }
    return ParseServiceEndpoint(item);
  }
  spdlog::error("support_services entry name={} not found", name);
  return std::nullopt;
}

}  // namespace

std::optional<QtradeEngineBootstrapConfig> ParseQtradeEngineBootstrapConfig(const nlohmann::json& config_node) {
  if (!config_node.is_object()) {
    spdlog::error("engine config must be an object");
    return std::nullopt;
  }
  if (!config_node.contains("config") || !config_node.at("config").is_object()) {
    spdlog::error("config missing or not an object");
    return std::nullopt;
  }
  if (!config_node.contains("support_services") || !config_node.at("support_services").is_array()) {
    spdlog::error("support_services missing or not an array");
    return std::nullopt;
  }

  const auto& process = config_node.at("config");
  if (!process.contains("identity") || !process.at("identity").is_object()) {
    spdlog::error("config.identity missing");
    return std::nullopt;
  }
  if (!process.contains("strategy") || !process.at("strategy").is_object()) {
    spdlog::error("config.strategy missing");
    return std::nullopt;
  }

  const auto& identity = process.at("identity");
  const auto& strategy = process.at("strategy");
  const auto& support_services = config_node.at("support_services");

  QtradeEngineBootstrapConfig out;
  out.config.log_dir = process.value("log_dir", out.config.log_dir);
  out.config.log_filename = process.value("log_filename", out.config.log_filename);
  if (out.config.log_dir.empty() || out.config.log_filename.empty()) {
    spdlog::error("config.log_dir/log_filename required");
    return std::nullopt;
  }

  out.config.identity.tenant_id = identity.value("tenant_id", "");
  out.config.identity.engine_id = identity.value("engine_id", "");
  out.config.identity.account_id = identity.value("account_id", "");
  if (out.config.identity.tenant_id.empty() || out.config.identity.engine_id.empty() ||
      out.config.identity.account_id.empty()) {
    spdlog::error("config.identity.tenant_id/engine_id/account_id required");
    return std::nullopt;
  }

  out.config.strategy.plugin_dir = strategy.value("plugin_dir", "");
  if (out.config.strategy.plugin_dir.empty()) {
    spdlog::error("config.strategy.plugin_dir required");
    return std::nullopt;
  }

  const auto config_service = ParseSupportServiceByName(support_services, "config_service");
  const auto account_service = ParseSupportServiceByName(support_services, "account_service");
  const auto account_risk_service = ParseSupportServiceByName(support_services, "account_risk_service");
  if (!config_service.has_value() || !account_service.has_value() || !account_risk_service.has_value()) {
    return std::nullopt;
  }

  out.support_services.config_service = config_service.value();
  out.support_services.account_service = account_service.value();
  out.support_services.account_risk_service = account_risk_service.value();

  if (out.support_services.account_risk_service.enabled &&
      out.support_services.account_risk_service.timeout_ms <= 0) {
    spdlog::error("account_risk_service.enabled but timeout_ms invalid");
    return std::nullopt;
  }
  return out;
}

}  // namespace qtrade::common::config
