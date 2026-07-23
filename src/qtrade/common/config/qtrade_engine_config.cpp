/// @file      qtrade_engine_config.cpp
/// @brief     QtradeEngineConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_engine_config.hpp"

#include "spdlog/spdlog.h"

namespace qtrade::common::config {
namespace {

/// @brief 解析 support_services 中的指定端点
/// @param support_services support_services 对象
/// @param key 服务键名
/// @return 解析结果
[[nodiscard]] std::optional<ServiceConfig> ParseRequiredSupportService(const nlohmann::json& support_services,
                                                                       const char* key) {
  if (!support_services.contains(key) || !support_services.at(key).is_object()) {
    spdlog::error("support_services.{} missing or not an object", key);
    return std::nullopt;
  }
  return ParseServiceEndpoint(support_services.at(key));
}

}  // namespace

std::optional<QtradeEngineConfig> ParseQtradeEngineConfig(const nlohmann::json& config_node) {
  if (!config_node.is_object()) {
    spdlog::error("engine config must be an object");
    return std::nullopt;
  }
  if (!config_node.contains("identity") || !config_node.at("identity").is_object()) {
    spdlog::error("identity missing");
    return std::nullopt;
  }
  if (!config_node.contains("support_services") || !config_node.at("support_services").is_object()) {
    spdlog::error("support_services missing");
    return std::nullopt;
  }

  const auto& identity = config_node.at("identity");
  const auto& support_services = config_node.at("support_services");

  QtradeEngineConfig config;
  config.identity.tenant_id = identity.value("tenant_id", "");
  config.identity.engine_id = identity.value("engine_id", "");
  config.identity.account_id = identity.value("account_id", "");
  if (config.identity.tenant_id.empty() || config.identity.engine_id.empty() || config.identity.account_id.empty()) {
    spdlog::error("identity.tenant_id/engine_id/account_id required");
    return std::nullopt;
  }

  const auto config_service = ParseRequiredSupportService(support_services, "config_service");
  const auto account_service = ParseRequiredSupportService(support_services, "account_service");
  const auto account_risk_service = ParseRequiredSupportService(support_services, "account_risk_service");
  const auto log_service = ParseRequiredSupportService(support_services, "log_service");
  if (!config_service.has_value() || !account_service.has_value() || !account_risk_service.has_value() ||
      !log_service.has_value()) {
    return std::nullopt;
  }

  config.support_services.config_service = config_service.value();
  config.support_services.account_service = account_service.value();
  config.support_services.account_risk_service = account_risk_service.value();
  config.support_services.log_service = log_service.value();

  if (config.support_services.account_risk_service.enabled &&
      config.support_services.account_risk_service.timeout_ms <= 0) {
    spdlog::error("account_risk_service.enabled but timeout_ms invalid");
    return std::nullopt;
  }
  if (!config.support_services.log_service.Extension("topic").has_value()) {
    spdlog::error("log_service.topic required");
    return std::nullopt;
  }
  return config;
}

}  // namespace qtrade::common::config
