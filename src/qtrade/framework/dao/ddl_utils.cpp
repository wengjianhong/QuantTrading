/// @file      ddl_utils.cpp
/// @brief     DDL 公共工具实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/dao/ddl_utils.hpp"

namespace qtrade::framework::dao {

ErrorCode EnsureTableSchema(cpputils::database::IConnection* connection, const ITableDdl& schema) {
  if (connection == nullptr || !connection->IsConnected()) {
    return ErrorCode::kSystemError;
  }

  // 1. 执行建表 SQL
  for (const auto& sql : schema.GetCreateTableSqls()) {
    if (!connection->Execute(sql)) {
      return ErrorCode::kSystemError;
    }
  }

  // 2. 创建索引
  for (const auto& sql : schema.GetIndexSqls()) {
    if (!connection->Execute(sql)) {
      return ErrorCode::kSystemError;
    }
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::framework::dao
