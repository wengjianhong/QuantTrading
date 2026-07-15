/// @file support_database_service_config.hpp
/// @brief 所有“gRPC + 数据库”支撑服务共用的 L0 引导配置。
#ifndef QTRADE_COMMON_CONFIG_SUPPORT_DATABASE_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_SUPPORT_DATABASE_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/common/config/grpc_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

struct SupportDatabaseServiceConfig {
  GrpcConfig grpc;
  DatabaseConfig database;
};

[[nodiscard]] std::optional<SupportDatabaseServiceConfig> ParseSupportDatabaseServiceConfig(const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_SUPPORT_DATABASE_SERVICE_CONFIG_HPP_
