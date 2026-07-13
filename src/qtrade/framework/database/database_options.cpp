/// @file      database_options.cpp
/// @brief     JSON database section parsing implementation
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/database/database_options.hpp"

#include <cpputils/database/config.hpp>
#include <cpputils/database/database_types.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <map>
#include <utility>

namespace qtrade::common {
namespace {

void ParseSociOptions(const nlohmann::json& node, std::map<std::string, std::string>& out) {
  if (!node.contains("soci_options") || !node["soci_options"].is_object()) {
    return;
  }
  for (auto it = node["soci_options"].begin(); it != node["soci_options"].end(); ++it) {
    out[it.key()] = it.value().get<std::string>();
  }
}

time_t ParseSeconds(const nlohmann::json& node, const char* key) {
  if (!node.contains(key)) {
    return 0;
  }
  return static_cast<time_t>(node[key].get<std::int64_t>());
}

cpputils::database::ConnectionConfig BuildLegacyConnectionConfig(const nlohmann::json& database,
                                                                 cpputils::database::DatabaseType type) {
  cpputils::database::ConnectionConfig options;
  options.database_type = type;
  options.conn_string = database["conn_string"].get<std::string>();
  ParseSociOptions(database, options.soci_options);
  return options;
}

cpputils::database::ConnectionConfig BuildSqliteConnectionConfig(const nlohmann::json& db_config) {
  cpputils::database::SqliteConfig config;
  if (db_config.contains("database_path")) {
    config.database_path = db_config["database_path"].get<std::string>();
  }
  config.busy_timeout = ParseSeconds(db_config, "busy_timeout");
  ParseSociOptions(db_config, config.soci_options);
  return cpputils::database::ConnectionConfig{config};
}

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

cpputils::database::ConnectionType ParsePostgreSqlConnectionType(const std::string& name) {
  if (name == "unix") {
    return cpputils::database::ConnectionType::kUnix;
  }
  return cpputils::database::ConnectionType::kTcp;
}

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

const nlohmann::json& DbConfigNode(const nlohmann::json& database) {
  static const nlohmann::json kEmpty = nlohmann::json::object();
  if (database.contains("config") && database["config"].is_object()) {
    return database["config"];
  }
  return kEmpty;
}

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

void ParsePoolOptions(const nlohmann::json& database, DatabaseOptions& options) {
  if (!database.contains("pool") || !database["pool"].is_object()) {
    return;
  }

  const auto& pool = database["pool"];
  if (!pool.value("enabled", false)) {
    return;
  }

  const std::size_t pool_size = pool.contains("size") ? pool["size"].get<std::size_t>() : 4;
  cpputils::database::ConnectionPoolConfig pool_opts{options.connection, pool_size};
  pool_opts.lease_timeout = ParseSeconds(pool, "lease_timeout");
  options.pool = std::move(pool_opts);
}

}  // namespace

DatabaseOptions ParseDatabaseOptions(const std::string& json_path) {
  DatabaseOptions options;

  std::ifstream ifs(json_path);
  if (!ifs.is_open()) {
    return options;
  }

  nlohmann::json root;
  try {
    ifs >> root;
  } catch (const nlohmann::json::exception& ex) {
    spdlog::warn("[DatabaseOptions] invalid JSON in {}: {}", json_path, ex.what());
    return options;
  }

  if (!root.contains("database") || !root["database"].is_object()) {
    return options;
  }

  const auto& database = root["database"];
  options.enabled = database.value("enabled", false);
  if (!options.enabled) {
    return options;
  }

  options.connection = BuildConnectionConfig(database);
  ParsePoolOptions(database, options);
  return options;
}

}  // namespace qtrade::common
