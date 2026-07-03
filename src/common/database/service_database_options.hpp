/// @file      service_database_options.hpp
/// @brief     支撑服务 JSON 中 database 段的解析（config / account 等共用）
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SERVICE_DATABASE_OPTIONS_HPP_
#define QTRADE_COMMON_SERVICE_DATABASE_OPTIONS_HPP_

#include <cpputils/database/config.hpp>

#include <optional>
#include <string>

namespace qtrade::service {

/// @brief 支撑服务数据库连接选项（对应各服务 JSON 的 database 段）
struct DatabaseOptions {
  bool enabled = false;
  cpp_utils::database::ConnectionOptions connection;
  std::optional<cpp_utils::database::ConnectionPoolOptions> pool;
};

/// @brief 从服务配置文件解析 database 段
[[nodiscard]] DatabaseOptions ParseServiceDatabaseOptions(const std::string& json_path);

}  // namespace qtrade::service

#endif  // QTRADE_COMMON_SERVICE_DATABASE_OPTIONS_HPP_
