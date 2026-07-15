/// @file      database_config.hpp
/// @brief     JSON "database" 段配置结构与解析
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_DATABASE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_DATABASE_CONFIG_HPP_

#include <cpputils/database/config.hpp>

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 数据库连接配置（对应 JSON "database" 段）
struct DatabaseConfig {
  /// 是否启用数据库
  bool enabled = false;
  /// 连接参数
  cpputils::database::ConnectionConfig connection;
  /// 连接池；未启用时为空
  std::optional<cpputils::database::ConnectionPoolConfig> pool;
};

/// @brief 从 "database" 段对象解析（不含外层 database 键）
/// @param database_node 形如 { enabled, type, config, pool } 的对象
/// @return 解析结果
[[nodiscard]] DatabaseConfig ParseDatabaseConfigFromSection(const nlohmann::json& database_node);

/// @brief 从配置根对象解析 "database" 段
/// @param root JSON 根对象
/// @return 解析结果；无 database 段或未启用时 enabled=false
[[nodiscard]] DatabaseConfig ParseDatabaseConfigFromRoot(const nlohmann::json& root);

/// @brief 从 JSON 字符串解析 "database" 段
/// @param json JSON 文本（完整配置或仅含 database 的文档）
/// @param out 输出配置
/// @return true 表示 JSON 合法且解析成功
[[nodiscard]] bool ParseDatabaseConfig(const std::string& json, DatabaseConfig& out);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_DATABASE_CONFIG_HPP_
