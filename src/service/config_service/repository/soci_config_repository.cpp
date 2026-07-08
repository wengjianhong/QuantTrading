/// @file      soci_config_repository.cpp
/// @brief     SociConfigRepository 实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "service/config_service/repository/soci_config_repository.hpp"

#include "service/config_service/repository/engine_config_codec.hpp"

#include <cpputils/database/database.hpp>

#include <spdlog/spdlog.h>

#include <sstream>
#include <utility>

namespace qtrade::service {
namespace {

constexpr const char* kEnsureSchemaSql = R"(
CREATE TABLE IF NOT EXISTS engine_config (
  tenant_id TEXT NOT NULL,
  engine_id TEXT NOT NULL,
  version INTEGER NOT NULL,
  payload TEXT NOT NULL,
  PRIMARY KEY (tenant_id, engine_id)
);
)";

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

}  // namespace

SociConfigRepository::SociConfigRepository(const qtrade::common::DatabaseOptions& options) {
  if (options.pool.has_value()) {
    pool_ = cpp_utils::database::CreateConnectionPool();
    if (const auto rc = pool_->Open(*options.pool); rc != cpp_utils::database::Error::kSuccess) {
      spdlog::error("[SociConfigRepository] open pool failed");
      pool_.reset();
      return;
    }
    connection_ = pool_->Acquire();
    if (!connection_) {
      spdlog::error("[SociConfigRepository] acquire connection from pool failed");
    }
    return;
  }

  auto owned = std::make_unique<cpp_utils::database::Connection>(options.connection);
  if (const auto rc = owned->Connect(); rc != cpp_utils::database::Error::kSuccess) {
    spdlog::error("[SociConfigRepository] connect failed: {}", owned->LastError());
  }
  connection_ = std::move(owned);
}

SociConfigRepository::~SociConfigRepository() {
  connection_.reset();
  if (pool_) {
    pool_->Close();
    pool_.reset();
  }
}

bool SociConfigRepository::IsReady() const { return connection_ != nullptr && connection_->IsConnected(); }

ErrorCode SociConfigRepository::EnsureSchema() {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }
  const auto rc = connection_->Execute(kEnsureSchemaSql);
  if (rc != cpp_utils::database::Error::kSuccess) {
    spdlog::error("[SociConfigRepository] ensure schema failed: {}", connection_->LastError());
  }
  return MapDbError(rc);
}

ErrorCode SociConfigRepository::Load(const ConfigScope& scope,
                                     qtrade::config::v1::EngineConfig& config,
                                     std::uint64_t& version) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  config.Clear();
  version = 0;

  std::ostringstream sql;
  sql << "SELECT version, payload FROM engine_config WHERE tenant_id = '" << EscapeSqlLiteral(scope.tenant_id)
      << "' AND engine_id = '" << EscapeSqlLiteral(scope.engine_id) << "'";

  auto [query_err, result] = connection_->Query(sql.str());
  if (query_err != cpp_utils::database::Error::kSuccess || result == nullptr) {
    spdlog::error("[SociConfigRepository] load failed: {}", connection_->LastError());
    return MapDbError(query_err);
  }

  const auto row = result->Fetch();
  if (!row.has_value()) {
    return ErrorCode::kNotFound;
  }

  std::optional<std::int64_t> version_opt;
  std::optional<std::string> payload_opt;
  if (const auto cell = row->get_value("version")) {
    version_opt = cell->as_int64();
  }
  if (const auto cell = row->get_value("payload")) {
    payload_opt = cell->as_string();
  }
  if (!version_opt.has_value() || !payload_opt.has_value()) {
    return ErrorCode::kSystemError;
  }

  version = static_cast<std::uint64_t>(version_opt.value());
  if (!EngineConfigFromJson(payload_opt.value(), config)) {
    spdlog::error(
      "[SociConfigRepository] invalid payload JSON for tenant={} engine={}", scope.tenant_id, scope.engine_id);
    return ErrorCode::kInternal;
  }
  return ErrorCode::kSuccess;
}

ErrorCode SociConfigRepository::Save(const ConfigScope& scope,
                                     const qtrade::config::v1::EngineConfig& config,
                                     std::uint64_t version) {
  if (!IsReady()) {
    return ErrorCode::kSystemError;
  }

  std::string payload;
  if (!EngineConfigToJson(config, payload)) {
    return ErrorCode::kInternal;
  }

  auto [begin_rc, tx] = connection_->BeginTransaction();
  if (begin_rc != cpp_utils::database::Error::kSuccess) {
    spdlog::error("[SociConfigRepository] begin transaction failed: {}", connection_->LastError());
    return MapDbError(begin_rc);
  }

  std::ostringstream upsert_sql;
  upsert_sql << "UPDATE engine_config SET version = " << version << ", payload = '" << EscapeSqlLiteral(payload)
             << "' WHERE tenant_id = '" << EscapeSqlLiteral(scope.tenant_id) << "' AND engine_id = '"
             << EscapeSqlLiteral(scope.engine_id) << "'";

  cpp_utils::database::ExecuteResult update_result;
  if (const auto rc = connection_->Execute(upsert_sql.str(), &update_result);
      rc != cpp_utils::database::Error::kSuccess) {
    (void)tx.Rollback();
    spdlog::error("[SociConfigRepository] update failed: {}", connection_->LastError());
    return MapDbError(rc);
  }

  if (update_result.affected_rows == 0) {
    std::ostringstream insert_sql;
    insert_sql << "INSERT INTO engine_config(tenant_id, engine_id, version, payload) VALUES('"
               << EscapeSqlLiteral(scope.tenant_id) << "', '" << EscapeSqlLiteral(scope.engine_id) << "', " << version
               << ", '" << EscapeSqlLiteral(payload) << "')";
    if (const auto rc = connection_->Execute(insert_sql.str()); rc != cpp_utils::database::Error::kSuccess) {
      (void)tx.Rollback();
      spdlog::error("[SociConfigRepository] insert failed: {}", connection_->LastError());
      return MapDbError(rc);
    }
  }

  if (const auto rc = tx.Commit(); rc != cpp_utils::database::Error::kSuccess) {
    spdlog::error("[SociConfigRepository] commit failed: {}", connection_->LastError());
    return MapDbError(rc);
  }
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::service
