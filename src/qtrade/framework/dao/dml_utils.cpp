/// @file      dml_utils.cpp
/// @brief     DML 公共工具实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/dao/dml_utils.hpp"

#include "qtrade/framework/dao/sql_utils.hpp"

#include <sstream>

namespace qtrade::framework::dao {
namespace {

cpputils::database::IConnection* g_connection = nullptr;  ///< 由 SetConnection 注册的全局连接

/// @brief 将 KeyValues 转为 UPDATE SET 赋值片段
/// @param values SET 列值
/// @return 如 "col1 = 'v1', col2 = 'v2'"
[[nodiscard]] std::string BuildAssignments(const KeyValues& values) {
  std::ostringstream sql;
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index > 0) {
      sql << ", ";
    }
    sql << values[index].first << " = '" << EscapeSqlLiteral(values[index].second) << "'";
  }
  return sql.str();
}

/// @brief 将 KeyValues 转为 INSERT 列与值片段
/// @param values 待插入列值
/// @return 如 "(col1, col2) VALUES ('v1', 'v2')"
[[nodiscard]] std::string BuildInsertColumns(const KeyValues& values) {
  std::ostringstream columns;
  std::ostringstream placeholders;
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index > 0) {
      columns << ", ";
      placeholders << ", ";
    }
    columns << values[index].first;
    placeholders << "'" << EscapeSqlLiteral(values[index].second) << "'";
  }
  std::ostringstream sql;
  sql << "(" << columns.str() << ") VALUES (" << placeholders.str() << ")";
  return sql.str();
}

[[nodiscard]] std::string LastDbErrorMessage() {
  if (g_connection == nullptr) {
    return {};
  }
  return g_connection->LastError().message;
}

template <typename T>
[[nodiscard]] Result<T> DbFailureResult() {
  return Result<T>{ErrorCode::kSystemError, LastDbErrorMessage()};
}

}  // namespace

void SetConnection(cpputils::database::IConnection* connection) {
  g_connection = connection;
}

cpputils::database::IConnection* GetConnection() {
  return g_connection;
}

void AddTextValue(KeyValues& out, const char* column, const std::optional<std::string>& value) {
  if (value.has_value()) {
    out.emplace_back(column, value.value());
  }
}

void AddInt64Value(KeyValues& out, const char* column, const std::optional<std::int64_t>& value) {
  if (value.has_value()) {
    out.emplace_back(column, std::to_string(value.value()));
  }
}

void AddUInt64Value(KeyValues& out, const char* column, const std::optional<std::uint64_t>& value) {
  if (value.has_value()) {
    out.emplace_back(column, std::to_string(value.value()));
  }
}

void AssignTextField(const cpputils::database::IResultRow& row, const char* column, std::optional<std::string>& field) {
  if (const auto cell = row.get_value(column)) {
    if (const auto v = cell->as_string()) {
      field = v.value();
    }
  }
}

void AssignInt64Field(const cpputils::database::IResultRow& row,
                      const char* column,
                      std::optional<std::int64_t>& field) {
  if (const auto cell = row.get_value(column)) {
    if (const auto v = cell->as_int64()) {
      field = v.value();
    }
  }
}

void AssignUInt64Field(const cpputils::database::IResultRow& row,
                       const char* column,
                       std::optional<std::uint64_t>& field) {
  if (const auto cell = row.get_value(column)) {
    if (const auto v = cell->as_int64()) {
      field = static_cast<std::uint64_t>(v.value());
    }
  }
}

std::string BuildWhereSql(const KeyValues& where_values) {
  if (where_values.empty()) {
    return {};
  }

  std::ostringstream sql;
  bool has_where = false;
  for (const auto& [column, value] : where_values) {
    AppendStringEq(sql, column.c_str(), value, has_where);
  }
  return sql.str();
}

Result<std::int64_t> InsertRow(const std::string& table, const KeyValues& values) {
  auto* connection = GetConnection();
  if (connection == nullptr || !connection->IsConnected() || values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  std::ostringstream sql;
  sql << "INSERT INTO " << table << " " << BuildInsertColumns(values);
  if (!connection->Execute(sql.str())) {
    return DbFailureResult<std::int64_t>();
  }
  return Result<std::int64_t>{ErrorCode::kSuccess, "", 1};
}

Result<std::int64_t> DeleteRows(const std::string& table, const KeyValues& where_values) {
  auto* connection = GetConnection();
  if (connection == nullptr || !connection->IsConnected() || where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  std::ostringstream sql;
  sql << "DELETE FROM " << table << BuildWhereSql(where_values);

  std::int64_t affected_rows = 0;
  if (!connection->Execute(sql.str(), &affected_rows)) {
    return DbFailureResult<std::int64_t>();
  }
  return Result<std::int64_t>{ErrorCode::kSuccess, "", affected_rows};
}

Result<std::int64_t> UpdateRows(const std::string& table, const KeyValues& values, const KeyValues& where_values) {
  auto* connection = GetConnection();
  if (connection == nullptr || !connection->IsConnected() || values.empty() || where_values.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  std::ostringstream sql;
  sql << "UPDATE " << table << " SET " << BuildAssignments(values) << BuildWhereSql(where_values);

  std::int64_t affected_rows = 0;
  if (!connection->Execute(sql.str(), &affected_rows)) {
    return DbFailureResult<std::int64_t>();
  }
  return Result<std::int64_t>{ErrorCode::kSuccess, "", affected_rows};
}

Result<std::int64_t> CountRows(const std::string& table, const KeyValues& where_values) {
  auto* connection = GetConnection();
  if (connection == nullptr || !connection->IsConnected()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  std::ostringstream sql;
  sql << "SELECT COUNT(*) AS cnt FROM " << table << BuildWhereSql(where_values);

  auto query_result = connection->Query(sql.str());
  if (query_result == nullptr) {
    return DbFailureResult<std::int64_t>();
  }
  const auto row = query_result->Fetch();
  if (!row.has_value()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  if (const auto cell = (*row)->get_value("cnt")) {
    if (const auto v = cell->as_int64()) {
      return Result<std::int64_t>{ErrorCode::kSuccess, "", v.value()};
    }
  }
  return Result<std::int64_t>{ErrorCode::kSystemError};
}

Result<std::unique_ptr<cpputils::database::IResultSet>> SelectRows(const std::string& table,
                                                                   const KeyValues& where_values) {
  auto* connection = GetConnection();
  if (connection == nullptr || !connection->IsConnected()) {
    return Result<std::unique_ptr<cpputils::database::IResultSet>>{ErrorCode::kSystemError};
  }

  std::ostringstream sql;
  sql << "SELECT * FROM " << table << BuildWhereSql(where_values);

  auto query_result = connection->Query(sql.str());
  if (query_result == nullptr) {
    return DbFailureResult<std::unique_ptr<cpputils::database::IResultSet>>();
  }
  return Result<std::unique_ptr<cpputils::database::IResultSet>>{ErrorCode::kSuccess, "", std::move(query_result)};
}

Result<std::int64_t> TruncateRows(const std::string& table) {
  auto* connection = GetConnection();
  if (connection == nullptr || !connection->IsConnected()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  std::ostringstream sql;
  sql << "DELETE FROM " << table;

  std::int64_t affected_rows = 0;
  if (!connection->Execute(sql.str(), &affected_rows)) {
    return DbFailureResult<std::int64_t>();
  }
  return Result<std::int64_t>{ErrorCode::kSuccess, "", affected_rows};
}

}  // namespace qtrade::framework::dao
