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

  QtradeEngineConfig config;
  if (root->contains("config_service")) {
    config.config_service = (*root)["config_service"].get<std::string>();
  } else {
    spdlog::error("config_service not found");
    return std::nullopt;
  }

  if (root->contains("account_service")) {
    config.account_service = (*root)["account_service"].get<std::string>();
  } else {
    spdlog::error("account_service not found");
    return std::nullopt;
  }

  if (root->contains("tenant_id")) {
    config.tenant_id = (*root)["tenant_id"].get<std::string>();
  } else {
    spdlog::error("tenant_id not found");
    return std::nullopt;
  }

  if (root->contains("engine_id")) {
    config.engine_id = (*root)["engine_id"].get<std::string>();
  } else {
    spdlog::error("engine_id not found");
    return std::nullopt;
  }

  if (root->contains("account_id")) {
    config.account_id = (*root)["account_id"].get<std::string>();
  } else {
    spdlog::error("account_id not found");
    return std::nullopt;
  }

  if (root->contains("log_topic")) {
    config.log_topic = (*root)["log_topic"].get<std::string>();
  } else {
    spdlog::error("log_topic not found");
    return std::nullopt;
  }

  if (root->contains("monitor_endpoint")) {
    config.monitor_endpoint = (*root)["monitor_endpoint"].get<std::string>();
  } else {
    spdlog::error("monitor_endpoint not found");
    return std::nullopt;
  }
  return config;
}

}  // namespace qtrade::common::config
