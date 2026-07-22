/// @file      engine_config.cpp
/// @brief     engine_config 表 DAO 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/dao/config_service/engine/engine_config.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::framework::dao {
namespace {

/// @brief 建表 SQL 脚本
const std::string kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS engine_config (
  tenant_id TEXT NOT NULL COMMENT '租户 ID',
  engine_id TEXT NOT NULL COMMENT '引擎 ID',
  version INTEGER NOT NULL COMMENT '配置版本号',
  payload TEXT NOT NULL COMMENT 'JSON 配置载荷',
  PRIMARY KEY (tenant_id, engine_id)
);
)";

/// @brief 逻辑数据库名
const std::string kDatabaseName = "config";

/// @brief 逻辑表名
const std::string kTableName = "engine_config";

/// @brief 建表 SQL 列表
const std::vector<std::string> kCreateTableSqls = {kCreateTableSql};

/// @brief 索引 SQL 列表
const std::vector<std::string> kIndexSqls = {};

}  // namespace

const std::string& EngineConfig::DatabaseName() const {
  return kDatabaseName;
}

const std::string& EngineConfig::TableName() const {
  return kTableName;
}

const std::vector<std::string>& EngineConfig::GetCreateTableSqls() const {
  return kCreateTableSqls;
}

const std::vector<std::string>& EngineConfig::GetIndexSqls() const {
  return kIndexSqls;
}

/// @brief 将 EngineConfigRecord 转为 KeyValues
KeyValues BuildEngineConfigValues(const EngineConfigRecord& record) {
  KeyValues values;
  AddTextValue(values, "tenant_id", record.tenant_id);
  AddTextValue(values, "engine_id", record.engine_id);
  AddUInt64Value(values, "version", record.version);
  AddTextValue(values, "payload", record.payload);
  return values;
}

Result<std::int64_t> EngineConfig::Insert(cpputils::database::IConnection& connection,
                                          const std::vector<EngineConfigRecord>& records) {
  if (records.empty()) {
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }

  // 1. 逐条转换并写入
  std::int64_t affected = 0;
  for (const auto& record : records) {
    const KeyValues values = BuildEngineConfigValues(record);
    if (values.empty()) {
      return Result<std::int64_t>{ErrorCode::kSystemError};
    }
    const auto result = InsertRow(connection, TableName(), values);
    if (result.error_code != ErrorCode::kSuccess) {
      spdlog::error("[EngineConfig] insert failed");
      return result;
    }
    affected += result.data.value_or(0);
  }
  return Result<std::int64_t>{ErrorCode::kSuccess, "", affected};
}

Result<std::int64_t> EngineConfig::Delete(cpputils::database::IConnection& connection,
                                          const EngineConfigRecord& where_conditions) {
  const KeyValues where_values = BuildEngineConfigValues(where_conditions);
  if (where_values.empty()) {
    spdlog::error("[EngineConfig] delete failed: empty where");
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return DeleteRows(connection, TableName(), where_values);
}

Result<std::int64_t> EngineConfig::BatchDelete(cpputils::database::IConnection&, const std::vector<std::int64_t>&) {
  spdlog::error("[EngineConfig] batch delete unsupported: composite primary key");
  return Result<std::int64_t>{ErrorCode::kInternalError, "composite primary key"};
}

Result<std::int64_t> EngineConfig::Update(cpputils::database::IConnection& connection,
                                          const EngineConfigRecord& record,
                                          const EngineConfigRecord& where_conditions) {
  const KeyValues values = BuildEngineConfigValues(record);
  const KeyValues where_values = BuildEngineConfigValues(where_conditions);
  if (values.empty() || where_values.empty()) {
    spdlog::error("[EngineConfig] update failed: empty values or where");
    return Result<std::int64_t>{ErrorCode::kSystemError};
  }
  return UpdateRows(connection, TableName(), values, where_values);
}

Result<std::int64_t> EngineConfig::Count(cpputils::database::IConnection& connection,
                                         const EngineConfigRecord& where_conditions) {
  return CountRows(connection, TableName(), BuildEngineConfigValues(where_conditions));
}

Result<std::vector<EngineConfigRecord>> EngineConfig::Select(cpputils::database::IConnection& connection,
                                                             const EngineConfigRecord& where_conditions) {
  // 1. 按条件查询
  auto query_result = SelectRows(connection, TableName(), BuildEngineConfigValues(where_conditions));
  if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value()) {
    spdlog::error("[EngineConfig] select failed: {}", connection.LastError().message);
    return Result<std::vector<EngineConfigRecord>>{query_result.error_code};
  }

  // 2. 逐行映射为记录
  std::vector<EngineConfigRecord> rows;
  while (const auto row = query_result.data.value()->Fetch()) {
    rows.push_back(BuildEngineConfigRecord(**row));
  }
  return Result<std::vector<EngineConfigRecord>>{ErrorCode::kSuccess, "", std::move(rows)};
}

Result<std::int64_t> EngineConfig::Truncate(cpputils::database::IConnection& connection) {
  return TruncateRows(connection, TableName());
}

}  // namespace qtrade::framework::dao
