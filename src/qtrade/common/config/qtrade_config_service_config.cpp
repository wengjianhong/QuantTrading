/// @file      qtrade_config_service_config.cpp
/// @brief     QtradeConfigServiceConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_config_service_config.hpp"

#include "qtrade/common/json/json_util.hpp"
#include "spdlog/spdlog.h"

namespace qtrade::common::config {

std::optional<QtradeConfigServiceConfig> ParseQtradeConfigServiceConfig(const std::string& json) {
  const auto root = ParseJsonString(json);
  if (!root.has_value()) {
    spdlog::error("parse json failed");
    return std::nullopt;
  }
  const auto& root_json = root.value();
  const auto grpc = ParseServiceConfig(root_json);
  if (!grpc.has_value()) {
    spdlog::error("parse grpc config failed");
    return std::nullopt;
  }

  QtradeConfigServiceConfig out;
  out.grpc = grpc.value();
  out.database = ParseDatabaseConfigFromRoot(root_json);
  return out;
}

}  // namespace qtrade::common::config
