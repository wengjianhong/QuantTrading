/// @file      service_config.cpp
/// @brief     ServiceConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/service_config.hpp"

#include "spdlog/spdlog.h"

#include <sstream>

namespace qtrade::common::config {
namespace {

constexpr const char* kHost = "host";
constexpr const char* kPort = "port";
constexpr const char* kEnabled = "enabled";
constexpr const char* kTimeoutMs = "timeout_ms";

/// @brief 将 JSON 标量转为扩展字段字符串
/// @param value JSON 值
/// @return 转换结果；对象/数组不支持则 nullopt
[[nodiscard]] std::optional<std::string> JsonScalarToString(const nlohmann::json& value) {
  if (value.is_string()) {
    return value.get<std::string>();
  }
  if (value.is_boolean()) {
    return value.get<bool>() ? std::string("true") : std::string("false");
  }
  if (value.is_number_integer()) {
    return std::to_string(value.get<std::int64_t>());
  }
  if (value.is_number_unsigned()) {
    return std::to_string(value.get<std::uint64_t>());
  }
  if (value.is_number_float()) {
    std::ostringstream oss;
    oss << value.get<double>();
    return oss.str();
  }
  if (value.is_null()) {
    return std::string{};
  }
  return std::nullopt;
}

}  // namespace

std::string ServiceConfig::Address() const {
  return host + ":" + std::to_string(port);
}

std::optional<std::string> ServiceConfig::Extension(const std::string& key) const {
  const auto it = extensions.find(key);
  if (it == extensions.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<ServiceConfig> ParseServiceEndpoint(const nlohmann::json& endpoint) {
  if (!endpoint.is_object()) {
    spdlog::error("service endpoint is not an object");
    return std::nullopt;
  }

  ServiceConfig config;
  if (endpoint.contains(kHost)) {
    if (!endpoint.at(kHost).is_string()) {
      spdlog::error("service endpoint host must be a string");
      return std::nullopt;
    }
    config.host = endpoint.at(kHost).get<std::string>();
  }
  if (endpoint.contains(kPort)) {
    if (!endpoint.at(kPort).is_number_integer()) {
      spdlog::error("service endpoint port must be an integer");
      return std::nullopt;
    }
    config.port = endpoint.at(kPort).get<int>();
  }
  if (endpoint.contains(kEnabled)) {
    if (!endpoint.at(kEnabled).is_boolean()) {
      spdlog::error("service endpoint enabled must be a boolean");
      return std::nullopt;
    }
    config.enabled = endpoint.at(kEnabled).get<bool>();
  }
  if (endpoint.contains(kTimeoutMs)) {
    if (!endpoint.at(kTimeoutMs).is_number_integer()) {
      spdlog::error("service endpoint timeout_ms must be an integer");
      return std::nullopt;
    }
    config.timeout_ms = endpoint.at(kTimeoutMs).get<int>();
  }

  for (const auto& [key, value] : endpoint.items()) {
    if (key == kHost || key == kPort || key == kEnabled || key == kTimeoutMs) {
      continue;
    }
    const auto text = JsonScalarToString(value);
    if (!text.has_value()) {
      spdlog::error("service endpoint extension '{}' must be a scalar", key);
      return std::nullopt;
    }
    config.extensions.emplace(key, text.value());
  }

  if (!config.IsConfigured()) {
    spdlog::error("service endpoint host/port invalid");
    return std::nullopt;
  }
  return config;
}

std::optional<ServiceConfig> ParseServiceConfig(const nlohmann::json& root) {
  if (!root.contains("grpc") || !root.at("grpc").is_object()) {
    spdlog::error("grpc listen config not found");
    return std::nullopt;
  }
  return ParseServiceEndpoint(root.at("grpc"));
}

}  // namespace qtrade::common::config
