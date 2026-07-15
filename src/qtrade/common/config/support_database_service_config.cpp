/// @file support_database_service_config.cpp
#include "qtrade/common/config/support_database_service_config.hpp"

#include "qtrade/common/json/json_util.hpp"

namespace qtrade::common::config {

std::optional<SupportDatabaseServiceConfig> ParseSupportDatabaseServiceConfig(const std::string& json) {
  const auto root = ParseJsonString(json);
  if (!root.has_value()) {
    return std::nullopt;
  }
  const auto& root_json = root.value();
  const auto grpc = ParseGrpcConfig(root_json);
  if (!grpc.has_value()) {
    return std::nullopt;
  }
  SupportDatabaseServiceConfig config;
  config.grpc = *grpc;
  config.database = ParseDatabaseConfigFromRoot(root_json);
  return config;
}

}  // namespace qtrade::common::config
