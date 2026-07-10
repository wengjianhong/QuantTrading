/// @file      database_options.hpp
/// @brief     JSON database section parsing (shared by services, engine, etc.)
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DATABASE_OPTIONS_HPP_
#define QTRADE_COMMON_DATABASE_OPTIONS_HPP_

#include <cpputils/database/config.hpp>

#include <optional>
#include <string>

namespace qtrade::common {

/// @brief Database connection options (maps to the "database" section in JSON config)
struct DatabaseOptions {
  bool enabled = false;
  cpputils::database::ConnectionConfig connection;
  std::optional<cpputils::database::ConnectionPoolConfig> pool;
};

/// @brief Parse the "database" section from a JSON config file
[[nodiscard]] DatabaseOptions ParseDatabaseOptions(const std::string& json_path);

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_DATABASE_OPTIONS_HPP_
