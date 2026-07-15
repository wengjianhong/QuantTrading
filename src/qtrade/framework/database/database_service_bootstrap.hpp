/// @file      database_service_bootstrap.hpp
/// @brief     带数据库连接的服务启动引导
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_
#define QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/common/file/text_file.hpp"
#include "qtrade/framework/dao/dml_utils.hpp"
#include "qtrade/framework/database/db_connection.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <cpputils/database/connection.hpp>

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace qtrade::common {

/// @brief 数据库连接启动上下文
struct DatabaseConnectionContext {
  /// 未启用或连接失败时为 nullptr
  std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection;
};

/// @brief 使用已解析的数据库配置连接、注册 DAO 并确保表结构
/// @param database_config 数据库配置（须 enabled）
/// @param ensure_schema 建表回调
/// @param log_tag 日志标识
template <typename EnsureSchemaFn>
[[nodiscard]] DatabaseConnectionContext BootstrapDatabaseConnection(const config::DatabaseConfig& database_config,
                                                                    EnsureSchemaFn&& ensure_schema,
                                                                    const std::string_view log_tag) {
  DatabaseConnectionContext context;

  if (!database_config.enabled) {
    spdlog::error("[{}] database disabled", log_tag);
    return context;
  }

  auto connection = std::make_shared<qtrade::framework::dao::DbConnectionHolder>(database_config);
  if (!connection->IsReady()) {
    spdlog::error("[{}] database connection failed", log_tag);
    return context;
  }

  qtrade::framework::dao::SetConnection(connection->Connection());
  if (const auto rc = ensure_schema(connection->Connection()); rc != ErrorCode::kSuccess) {
    spdlog::error("[{}] ensure schema failed", log_tag);
    return context;
  }

  context.connection = std::move(connection);
  spdlog::info("[{}] database ready", log_tag);
  return context;
}

/// @brief 从配置文件解析 database 段后启动连接
/// @param json_path 服务配置文件路径
/// @param ensure_schema 建表回调，接收底层 IConnection*
/// @param log_tag 日志标识
template <typename EnsureSchemaFn>
[[nodiscard]] DatabaseConnectionContext BootstrapDatabaseConnection(const std::string& json_path,
                                                                    EnsureSchemaFn&& ensure_schema,
                                                                    const std::string_view log_tag) {
  const auto json_text = ReadTextFile(json_path);
  if (!json_text.has_value()) {
    spdlog::error("[{}] load database config failed", log_tag);
    return DatabaseConnectionContext{};
  }
  config::DatabaseConfig database_config;
  if (!config::ParseDatabaseConfig(*json_text, database_config)) {
    spdlog::error("[{}] load database config failed", log_tag);
    return DatabaseConnectionContext{};
  }
  return BootstrapDatabaseConnection(database_config, std::forward<EnsureSchemaFn>(ensure_schema), log_tag);
}

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_
