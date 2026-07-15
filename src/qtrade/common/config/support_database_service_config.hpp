/// @file      support_database_service_config.hpp
/// @brief     “gRPC + 数据库”支撑服务共用的 L0 引导配置
/// @details   config / account / account-risk 等服务可复用本结构，避免重复解析 grpc 与 database 段
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_SUPPORT_DATABASE_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_SUPPORT_DATABASE_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/common/config/grpc_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 含 gRPC 监听与数据库连接的支撑服务引导配置
struct SupportDatabaseServiceConfig {
  /// gRPC 监听
  GrpcConfig grpc;
  /// 数据库连接与连接池
  DatabaseConfig database;
};

/// @brief 从 JSON 字符串解析 SupportDatabaseServiceConfig
/// @param json JSON 文本
/// @return 解析结果；JSON 非法或缺少 grpc 段时返回 nullopt
[[nodiscard]] std::optional<SupportDatabaseServiceConfig> ParseSupportDatabaseServiceConfig(const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_SUPPORT_DATABASE_SERVICE_CONFIG_HPP_
