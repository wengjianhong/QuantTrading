/// @file      soci_config_repository.cpp
/// @brief     SociConfigRepository 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "service/config_service/repository/soci_config_repository.hpp"

#include "service/config_service/repository/engine_config_codec.hpp"

#include "common/dao/ddl_utils.hpp"
#include "common/dao/dml_utils.hpp"
#include "dao/engine_config.hpp"

#include <cpputils/database/connection.hpp>

#include <spdlog/spdlog.h>

namespace qtrade::service {

SociConfigRepository::SociConfigRepository(const qtrade::common::DatabaseOptions& options) : connection_(options) {
  if (IsReady()) {
    qtrade::framework::dao::SetConnection(connection_.Connection());
  }
}

SociConfigRepository::~SociConfigRepository() = default;

bool SociConfigRepository::IsReady() const {
  return connection_.IsReady();
}

ErrorCode SociConfigRepository::EnsureSchema() {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  const auto& schema = qtrade::framework::dao::EngineConfig::Instance();
  const auto rc = qtrade::framework::dao::EnsureTableSchema(connection_.Connection(), schema);
  if (rc != ErrorCode::kSuccess) {
    spdlog::error("[SociConfigRepository] ensure schema failed");
  }
  return rc;
}

ErrorCode SociConfigRepository::Load(const ConfigScope& scope,
                                     qtrade::config::v1::EngineConfig& config,
                                     std::uint64_t& version) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  config.Clear();
  version = 0;

  qtrade::framework::dao::EngineConfigRecord where;
  where.tenant_id = scope.tenant_id;
  where.engine_id = scope.engine_id;

  const auto result = qtrade::framework::dao::EngineConfig::Instance().Select(where);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::error("[SociConfigRepository] load failed");
    return result.error_code;
  }
  if (!result.data.has_value() || result.data->empty()) {
    return ErrorCode::kNotFound;
  }

  const auto& row = result.data->front();
  version = row.version.value_or(0);
  if (!row.payload.has_value() || !EngineConfigFromJson(row.payload.value(), config)) {
    spdlog::error(
      "[SociConfigRepository] invalid payload JSON for tenant={} engine={}", scope.tenant_id, scope.engine_id);
    return ErrorCode::kInternal;
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociConfigRepository::Save(const ConfigScope& scope,
                                     const qtrade::config::v1::EngineConfig& config,
                                     const std::uint64_t version) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  // 1. 序列化 proto 为 JSON payload
  std::string payload;
  if (!EngineConfigToJson(config, payload)) {
    return ErrorCode::kInternal;
  }

  auto* connection = connection_.Connection();
  if (!connection->BeginTransaction()) {
    spdlog::error("[SociConfigRepository] begin transaction failed: {}", connection->LastError().message);
    return ErrorCode::kSystemError;
  }

  qtrade::framework::dao::EngineConfigRecord where;
  where.tenant_id = scope.tenant_id;
  where.engine_id = scope.engine_id;

  qtrade::framework::dao::EngineConfigRecord row;
  row.tenant_id = scope.tenant_id;
  row.engine_id = scope.engine_id;
  row.version = version;
  row.payload = payload;

  // 2. 先尝试 UPDATE，无行受影响则 INSERT
  auto& dao = qtrade::framework::dao::EngineConfig::Instance();
  const auto update_result = dao.Update(row, where);
  if (update_result.error_code != ErrorCode::kSuccess) {
    (void)connection->RollbackTransaction();
    return update_result.error_code;
  }
  if (!update_result.data.has_value() || update_result.data.value() == 0) {
    const auto insert_result = dao.Insert({row});
    if (insert_result.error_code != ErrorCode::kSuccess) {
      (void)connection->RollbackTransaction();
      spdlog::error("[SociConfigRepository] insert failed");
      return insert_result.error_code;
    }
  }

  // 3. 提交事务
  if (!connection->CommitTransaction()) {
    spdlog::error("[SociConfigRepository] commit failed: {}", connection->LastError().message);
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::service
