/// @file      database_config.cpp
/// @brief     DatabaseConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/database_config.hpp"

#include <cpputils/database/config.hpp>
#include <cpputils/database/database_types.hpp>

#include <spdlog/spdlog.h>

#include <map>
#include <utility>

namespace qtrade::common::config {
namespace {

/// @brief 解析 soci_options 对象到字符串映射
/// @param node 含可选 soci_options 字段的 JSON 对象
/// @param out 输出键值对（追加写入）
void ParseSociOptions(const nlohmann::json& node, std::map<std::string, std::string>& out) {
  if (!node.contains("soci_options") || !node["soci_options"].is_object()) {
    return;
  }
  for (auto it = node["soci_options"].begin(); it != node["soci_options"].end(); ++it) {
    out[it.key()] = it.value().get<std::string>();
  }
}

/// @brief 读取整型秒字段并转为 time_t
/// @param node JSON 对象
/// @param key 字段名
/// @return 秒数；字段缺失时返回 0
time_t ParseSeconds(const nlohmann::json& node, const char* key) {
  if (!node.contains(key)) {
    return 0;
  }
  return static_cast<time_t>(node[key].get<std::int64_t>());
}

/// @brief 从遗留 conn_string 字段构建连接配置
/// @param database database 段 JSON
/// @param type 数据库类型
/// @return 连接配置
cpputils::database::ConnectionConfig BuildLegacyConnectionConfig(const nlohmann::json& database,
                                                                 cpputils::database::DatabaseType type) {
  cpputils::database::ConnectionConfig options;
  options.database_type = type;
  options.conn_string = database["conn_string"].get<std::string>();
  ParseSociOptions(database, options.soci_options);
  return options;
}

/// @brief 从 config 对象构建 SQLite 连接配置
/// @param db_config database.config 对象
/// @return 连接配置
cpputils::database::ConnectionConfig BuildSqliteConnectionConfig(const nlohmann::json& db_config) {
  cpputils::database::SqliteConfig config;
  if (db_config.contains("database_path")) {
    config.database_path = db_config["database_path"].get<std::string>();
  }
  config.busy_timeout = ParseSeconds(db_config, "busy_timeout");
  ParseSociOptions(db_config, config.soci_options);
  return cpputils::database::ConnectionConfig{config};
}

/// @brief 从 config 对象构建 MySQL 连接配置
/// @param db_config database.config 对象
/// @return 连接配置
cpputils::database::ConnectionConfig BuildMySqlConnectionConfig(const nlohmann::json& db_config) {
  cpputils::database::MySqlConfig config;
  if (db_config.contains("host")) {
    config.host = db_config["host"].get<std::string>();
  }
  if (db_config.contains("port")) {
    config.port = db_config["port"].get<int>();
  }
  if (db_config.contains("user")) {
    config.user = db_config["user"].get<std::string>();
  }
  if (db_config.contains("password")) {
    config.password = db_config["password"].get<std::string>();
  }
  if (db_config.contains("database_name")) {
    config.database_name = db_config["database_name"].get<std::string>();
  }
  config.connect_timeout = ParseSeconds(db_config, "connect_timeout");
  ParseSociOptions(db_config, config.soci_options);
  return cpputils::database::ConnectionConfig{config};
}

/// @brief 解析 PostgreSQL 连接类型字符串
/// @param name "unix" 或其它（按 TCP）
/// @return ConnectionType
cpputils::database::ConnectionType ParsePostgreSqlConnectionType(const std::string& name) {
  if (name == "unix") {
    return cpputils::database::ConnectionType::kUnix;
  }
  return cpputils::database::ConnectionType::kTcp;
}

/// @brief 从 config 对象构建 PostgreSQL 连接配置
/// @param db_config database.config 对象
/// @return 连接配置
cpputils::database::ConnectionConfig BuildPostgreSqlConnectionConfig(const nlohmann::json& db_config) {
  cpputils::database::PostgreSqlConfig config;
  if (db_config.contains("host")) {
    config.host = db_config["host"].get<std::string>();
  }
  if (db_config.contains("port")) {
    config.port = db_config["port"].get<int>();
  }
  if (db_config.contains("user")) {
    config.user = db_config["user"].get<std::string>();
  }
  if (db_config.contains("password")) {
    config.password = db_config["password"].get<std::string>();
  }
  if (db_config.contains("database_name")) {
    config.database_name = db_config["database_name"].get<std::string>();
  }
  if (db_config.contains("socket_path")) {
    config.socket_path = db_config["socket_path"].get<std::string>();
  }
  if (db_config.contains("ssl_mode")) {
    config.ssl_mode = db_config["ssl_mode"].get<std::string>();
  }
  if (db_config.contains("connection_type")) {
    config.connection_type = ParsePostgreSqlConnectionType(db_config["connection_type"].get<std::string>());
  }
  config.connect_timeout = ParseSeconds(db_config, "connect_timeout");
  ParseSociOptions(db_config, config.soci_options);
  return cpputils::database::ConnectionConfig{config};
}

/// @brief 从 config 对象构建 Oracle 连接配置
/// @param db_config database.config 对象
/// @return 连接配置
cpputils::database::ConnectionConfig BuildOracleConnectionConfig(const nlohmann::json& db_config) {
  cpputils::database::OracleConfig config;
  if (db_config.contains("host")) {
    config.host = db_config["host"].get<std::string>();
  }
  if (db_config.contains("port")) {
    config.port = db_config["port"].get<int>();
  }
  if (db_config.contains("user")) {
    config.user = db_config["user"].get<std::string>();
  }
  if (db_config.contains("password")) {
    config.password = db_config["password"].get<std::string>();
  }
  if (db_config.contains("service_name")) {
    config.service_name = db_config["service_name"].get<std::string>();
  }
  config.connect_timeout = ParseSeconds(db_config, "connect_timeout");
  ParseSociOptions(db_config, config.soci_options);
  return cpputils::database::ConnectionConfig{config};
}

/// @brief 取得 database.config 子对象；缺失时返回空对象
/// @param database database 段 JSON
/// @return config 对象引用或静态空对象
const nlohmann::json& DbConfigNode(const nlohmann::json& database) {
  static const nlohmann::json kEmpty = nlohmann::json::object();
  if (database.contains("config") && database["config"].is_object()) {
    return database["config"];
  }
  return kEmpty;
}

/// @brief 按 type / conn_string / config 构建连接配置
/// @param database database 段 JSON
/// @return 连接配置
cpputils::database::ConnectionConfig BuildConnectionConfig(const nlohmann::json& database) {
  const auto type = cpputils::database::GetDatabaseTypeByName(database.value("type", ""));

  if (database.contains("conn_string")) {
    return BuildLegacyConnectionConfig(database, type);
  }

  const auto& db_config = DbConfigNode(database);

  switch (type) {
    case cpputils::database::DatabaseType::kMySql:
      return BuildMySqlConnectionConfig(db_config);
    case cpputils::database::DatabaseType::kPostgreSql:
      return BuildPostgreSqlConnectionConfig(db_config);
    case cpputils::database::DatabaseType::kOracle:
      return BuildOracleConnectionConfig(db_config);
    case cpputils::database::DatabaseType::kSqlite3:
    default:
      return BuildSqliteConnectionConfig(db_config);
  }
}

/// @brief 解析 pool 段并写入服务端连接池配置
/// @param database database 段 JSON
/// @param connection 已解析的单连接配置
/// @param options 配置输出
void ParsePoolOptions(const nlohmann::json& database,
                      const cpputils::database::ConnectionConfig& connection,
                      DatabaseConfig& options) {
  const auto& pool =
    database.contains("pool") && database.at("pool").is_object() ? database.at("pool") : nlohmann::json::object();
  std::size_t pool_size = 4;
  if (pool.contains("size")) {
    pool_size = pool.at("size").get<std::size_t>();
  }
  cpputils::database::ConnectionPoolConfig pool_opts{connection, pool_size};
  pool_opts.lease_timeout = ParseSeconds(pool, "lease_timeout");
  options.pool = std::move(pool_opts);
}

}  // namespace

DatabaseConfig ParseDatabaseConfigFromSection(const nlohmann::json& database) {
  DatabaseConfig options;
  if (!database.is_object()) {
    return options;
  }

  options.enabled = database.value("enabled", false);
  if (!options.enabled) {
    return options;
  }

  ParsePoolOptions(database, BuildConnectionConfig(database), options);
  return options;
}

}  // namespace qtrade::common::config
