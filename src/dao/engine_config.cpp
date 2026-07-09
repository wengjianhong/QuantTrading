/// @file      engine_config.cpp
/// @brief     engine_config 表 DAO 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "dao/engine_config.hpp"

#include "common/dao/sql_utils.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::framework::dao {
namespace {

EngineConfig* g_mock_instance = nullptr;  ///< 测试 Mock 实例指针

constexpr const char* kCreateTableSql = R"(
CREATE TABLE IF NOT EXISTS engine_config (
  tenant_id TEXT NOT NULL,
  engine_id TEXT NOT NULL,
  version INTEGER NOT NULL,
  payload TEXT NOT NULL,
  PRIMARY KEY (tenant_id, engine_id)
);
)";

}  // namespace

EngineConfig& EngineConfig::Instance() {
  if (g_mock_instance != nullptr) {
    return *g_mock_instance;
  }
  static EngineConfig instance;
  return instance;
}

void EngineConfig::SetMockInstance(EngineConfig* mock_instance) { g_mock_instance = mock_instance; }

void EngineConfig::ClearMockInstance() { g_mock_instance = nullptr; }

const std::string& EngineConfig::TableName() const {
  static const std::string kName = "engine_config";
  return kName;
}

const std::vector<std::string>& EngineConfig::GetCreateTableSqls() const {
  static const std::vector<std::string> kSqls = {kCreateTableSql};
  return kSqls;
}

const std::vector<std::string>& EngineConfig::GetIndexSqls() const {
  static const std::vector<std::string> kEmpty;
  return kEmpty;
}

KeyValues BuildEngineConfigValues(const EngineConfigRecord& record) {
  KeyValues values;
  AddTextValue(values, "tenant_id", record.tenant_id);
  AddTextValue(values, "engine_id", record.engine_id);
  AddUInt64Value(values, "version", record.version);
  AddTextValue(values, "payload", record.payload);
  return values;
}

Result<std::int64_t> EngineConfig::Insert(const std::vector<EngineConfigRecord>& records) {
  if (records.empty()) {
    return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
  }

  std::int64_t affected = 0;
  for (const auto& record : records) {
    const KeyValues values = BuildEngineConfigValues(record);
    if (values.empty()) {
      return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
    }
    const auto result = InsertRow(TableName(), values);
    if (result.error_code != ErrorCode::kSuccess) {
      spdlog::error("[EngineConfig] insert failed");
      return result;
    }
    affected += result.data.value_or(0);
  }
  return Result<std::int64_t>{.data = affected};
}

Result<std::int64_t> EngineConfig::Delete(const EngineConfigRecord& where_conditions) {
  const KeyValues where_values = BuildEngineConfigValues(where_conditions);
  if (where_values.empty()) {
    spdlog::error("[EngineConfig] delete failed: empty where");
    return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
  }
  return DeleteRows(TableName(), where_values);
}

Result<std::int64_t> EngineConfig::BatchDelete(const std::vector<std::int64_t>&) {
  spdlog::error("[EngineConfig] batch delete unsupported: composite primary key");
  return Result<std::int64_t>{.error_code = ErrorCode::kInternal, .error_message = "composite primary key"};
}

Result<std::int64_t> EngineConfig::Update(const EngineConfigRecord& record, const EngineConfigRecord& where_conditions) {
  const KeyValues values = BuildEngineConfigValues(record);
  const KeyValues where_values = BuildEngineConfigValues(where_conditions);
  if (values.empty() || where_values.empty()) {
    spdlog::error("[EngineConfig] update failed: empty values or where");
    return Result<std::int64_t>{.error_code = ErrorCode::kSystemError};
  }
  return UpdateRows(TableName(), values, where_values);
}

Result<std::int64_t> EngineConfig::Count(const EngineConfigRecord& where_conditions) {
  return CountRows(TableName(), BuildEngineConfigValues(where_conditions));
}

Result<std::vector<EngineConfigRecord>> EngineConfig::Select(const EngineConfigRecord& where_conditions) {
  auto query_result = SelectRows(TableName(), BuildEngineConfigValues(where_conditions));
  if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value()) {
    auto* connection = GetConnection();
    spdlog::error("[EngineConfig] select failed: {}",
                  connection != nullptr ? connection->LastError().message : "no connection");
    return Result<std::vector<EngineConfigRecord>>{.error_code = query_result.error_code};
  }

  std::vector<EngineConfigRecord> rows;
  while (const auto row = query_result.data.value()->Fetch()) {
    rows.push_back(BuildEngineConfigRecord(*row));
  }
  return Result<std::vector<EngineConfigRecord>>{.data = std::move(rows)};
}

Result<std::int64_t> EngineConfig::Truncate() {
  return TruncateRows(TableName());
}

}  // namespace qtrade::framework::dao
