/// @file      database_config.hpp
/// @brief     JSON "database" 段配置结构与解析
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_DATABASE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_DATABASE_CONFIG_HPP_

#include <cpputils/database/config.hpp>

#include <nlohmann/json.hpp>

namespace qtrade::common::config {

/// @brief 数据库连接池配置（对应 JSON "database" 段）
struct DatabaseConfig {
  /// 是否启用数据库
  bool enabled = false;
  /// 连接池参数（服务端运行期始终使用连接池）
  cpputils::database::ConnectionPoolConfig pool;
};

/// @brief 从 "database" 段对象解析（不含外层 database 键）
/// @param database_node 形如 { enabled, type, config, pool } 的对象
/// @return 解析结果
[[nodiscard]] DatabaseConfig ParseDatabaseConfigFromSection(const nlohmann::json& database_node);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_DATABASE_CONFIG_HPP_
