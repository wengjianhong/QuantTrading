/// @file      grpc_config.cpp
/// @brief     GrpcConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/grpc_config.hpp"

#include "spdlog/spdlog.h"

namespace qtrade::common::config {

std::string GrpcConfig::ListenAddress() const {
  return host + ":" + std::to_string(port);
}

std::optional<GrpcConfig> ParseGrpcConfig(const nlohmann::json& root) {
  if (!root.contains("grpc") || !root["grpc"].is_object()) {
    spdlog::error("grpc config not found");
    return std::nullopt;
  }

  GrpcConfig config;
  const auto& grpc = root["grpc"];
  if (grpc.contains("host")) {
    config.host = grpc["host"].get<std::string>();
  }
  if (grpc.contains("port")) {
    config.port = grpc["port"].get<int>();
  }
  return config;
}

}  // namespace qtrade::common::config
