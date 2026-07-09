/// @file      sql_utils.cpp
/// @brief     DAO SQL 与 Result 工具实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "common/dao/sql_utils.hpp"

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

ErrorCode MapDbError(cpp_utils::database::Error error) {
  switch (error) {
    case cpp_utils::database::Error::kSuccess:
      return ErrorCode::kSuccess;
    case cpp_utils::database::Error::kNotFound:
      return ErrorCode::kNotFound;
    case cpp_utils::database::Error::kInvalidArgument:
      return ErrorCode::kInternal;
    case cpp_utils::database::Error::kNotConnected:
    case cpp_utils::database::Error::kConnectFailed:
    case cpp_utils::database::Error::kQueryFailed:
    case cpp_utils::database::Error::kExecuteFailed:
    case cpp_utils::database::Error::kTransactionFailed:
    default:
      return ErrorCode::kSystemError;
  }
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
