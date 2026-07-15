/// @file      qtrade_config_service_config.hpp
/// @brief     qtrade_config_service.json 配置结构
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_CONFIG_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_CONFIG_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/common/config/grpc_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 对应 config/qtrade_config_service.json
struct QtradeConfigServiceConfig {
  /// gRPC 监听
  GrpcConfig grpc;
  /// 数据库连接与连接池
  DatabaseConfig database;
};

/// @brief 从 JSON 字符串解析配置中心服务配置
/// @param json JSON 文本
/// @return 解析结果；JSON 非法或缺少 grpc 段时返回 nullopt
[[nodiscard]] std::optional<QtradeConfigServiceConfig> ParseQtradeConfigServiceConfig(const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_CONFIG_SERVICE_CONFIG_HPP_
