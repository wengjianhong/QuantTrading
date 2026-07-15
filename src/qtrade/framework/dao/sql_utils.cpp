/// @file      sql_utils.cpp
/// @brief     DAO SQL 与 Result 工具实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/dao/sql_utils.hpp"

namespace qtrade::framework::dao {

std::string EscapeSqlLiteral(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char ch : value) {
    if (ch == '\'') {
      escaped += "''";
    } else {
      escaped += ch;
    }
  }
  return escaped;
}

void AppendStringEq(std::ostringstream& sql, const char* column, const std::string& value, bool& has_where) {
  if (value.empty()) {
    return;
  }
  sql << (has_where ? " AND " : " WHERE ");
  has_where = true;
  sql << column << " = '" << EscapeSqlLiteral(value) << "'";
}

}  // namespace qtrade::framework::dao
