/// @file      qtrade_engine_config.cpp
/// @brief     QtradeEngineConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_engine_config.hpp"

#include "qtrade/common/json/json_util.hpp"
#include "spdlog/spdlog.h"

namespace qtrade::common::config {

std::optional<QtradeEngineConfig> ParseQtradeEngineConfig(const std::string& json) {
  const auto root = ParseJsonString(json);
  if (!root.has_value()) {
    spdlog::error("parse json failed");
    return std::nullopt;
  }

  const auto& root_json = root.value();
  QtradeEngineConfig config;
  if (root_json.contains("config_service")) {
    config.config_service = root_json.at("config_service").get<std::string>();
  } else {
    spdlog::error("config_service not found");
    return std::nullopt;
  }

  if (root_json.contains("account_service")) {
    config.account_service = root_json.at("account_service").get<std::string>();
  } else {
    spdlog::error("account_service not found");
    return std::nullopt;
  }

  if (root_json.contains("account_risk_enabled")) {
    config.account_risk_enabled = root_json.at("account_risk_enabled").get<bool>();
  }
  if (root_json.contains("account_risk_service")) {
    config.account_risk_service = root_json.at("account_risk_service").get<std::string>();
  }
  if (root_json.contains("account_risk_timeout_ms")) {
    config.account_risk_timeout_ms = root_json.at("account_risk_timeout_ms").get<int>();
  }
  if (config.account_risk_enabled && (config.account_risk_service.empty() || config.account_risk_timeout_ms <= 0)) {
    spdlog::error("account-risk is enabled but service address or timeout is invalid");
    return std::nullopt;
  }

  if (root_json.contains("tenant_id")) {
    config.tenant_id = root_json.at("tenant_id").get<std::string>();
  } else {
    spdlog::error("tenant_id not found");
    return std::nullopt;
  }

  if (root_json.contains("engine_id")) {
    config.engine_id = root_json.at("engine_id").get<std::string>();
  } else {
    spdlog::error("engine_id not found");
    return std::nullopt;
  }

  if (root_json.contains("account_id")) {
    config.account_id = root_json.at("account_id").get<std::string>();
  } else {
    spdlog::error("account_id not found");
    return std::nullopt;
  }

  if (root_json.contains("fallback_engine_config_path")) {
    config.fallback_engine_config_path = root_json.at("fallback_engine_config_path").get<std::string>();
  }
  if (root_json.contains("log_service")) {
    config.log_service = root_json.at("log_service").get<std::string>();
  }

  if (root_json.contains("log_topic")) {
    config.log_topic = root_json.at("log_topic").get<std::string>();
  } else {
    spdlog::error("log_topic not found");
    return std::nullopt;
  }

  if (root_json.contains("monitor_endpoint")) {
    config.monitor_endpoint = root_json.at("monitor_endpoint").get<std::string>();
  } else {
    spdlog::error("monitor_endpoint not found");
    return std::nullopt;
  }
  return config;
}

}  // namespace qtrade::common::config
