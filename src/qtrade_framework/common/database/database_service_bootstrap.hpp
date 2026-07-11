/// @file      database_service_bootstrap.hpp
/// @brief     带数据库仓储的服务启动引导（共享模板）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_
#define QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_

#include "qtrade_framework/common/database/database_options.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace qtrade::common {

/// @brief 带数据库仓储的服务启动上下文
template <typename RepositoryT>
struct DatabaseServiceContext {
  std::shared_ptr<RepositoryT> repository;  ///< 仓储；database 未启用或连接失败时为 nullptr
};

/// @brief 连接数据库、创建仓储并确保表结构
/// @param json_path 服务配置文件路径
/// @param create_repo 仓储工厂
/// @param log_tag 日志标识
template <typename RepositoryT>
[[nodiscard]] DatabaseServiceContext<RepositoryT> BootstrapDatabaseService(
  const std::string& json_path,
  const std::function<std::shared_ptr<RepositoryT>(const DatabaseOptions&)>& create_repo,
  const std::string_view log_tag) {
  DatabaseServiceContext<RepositoryT> context;

  const auto database_options = ParseDatabaseOptions(json_path);
  if (!database_options.enabled) {
    spdlog::error("[{}] database disabled", log_tag);
    return context;
  }

  context.repository = create_repo(database_options);
  if (!context.repository) {
    spdlog::error("[{}] create repository failed", log_tag);
    return context;
  }

  if (const auto rc = context.repository->EnsureSchema(); rc != ErrorCode::kSuccess) {
    spdlog::error("[{}] ensure schema failed", log_tag);
    context.repository.reset();
    return context;
  }

  spdlog::info("[{}] database ready", log_tag);
  return context;
}

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_DATABASE_DATABASE_SERVICE_BOOTSTRAP_HPP_
