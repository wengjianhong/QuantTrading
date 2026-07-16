/// @file      database_service_bootstrap.hpp
/// @brief     带数据库连接的服务启动引导
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_
#define QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_

#include "qtrade/common/config/database_config.hpp"
#include "qtrade/framework/database/db_connection.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <cpputils/database/connection.hpp>

#include <spdlog/spdlog.h>

#include <memory>
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

  if (const auto rc = ensure_schema(connection->Connection()); rc != ErrorCode::kSuccess) {
    spdlog::error("[{}] ensure schema failed", log_tag);
    return context;
  }

  context.connection = std::move(connection);
  spdlog::info("[{}] database ready", log_tag);
  return context;
}

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_
