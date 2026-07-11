/// @file      config_repository.cpp
/// @brief     配置仓储工厂
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/repository/config_repository.hpp"

#include "qtrade/service/config_service/repository/soci_config_repository.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::service {

namespace {

std::string NormalizeScopeField(const std::string& value) {
  return value.empty() ? "default" : value;
}

}  // namespace

ConfigScope MakeConfigScope(const qtrade::config::v1::GetConfigRequest& request) {
  return ConfigScope{
    .tenant_id = "default",
    .engine_id = NormalizeScopeField(request.engine_id()),
  };
}

ConfigScope MakeConfigScope(const qtrade::config::v1::SubscribeConfigRequest& request) {
  return ConfigScope{
    .tenant_id = "default",
    .engine_id = NormalizeScopeField(request.engine_id()),
  };
}

std::shared_ptr<IConfigRepository> CreateConfigRepository(const qtrade::common::DatabaseOptions& options) {
  if (!options.enabled) {
    return nullptr;
  }
  return std::make_shared<SociConfigRepository>(options);
}

qtrade::config::v1::ConfigSnapshot QueryConfigSnapshot(IConfigRepository* repository, const ConfigScope& scope) {
  qtrade::config::v1::ConfigSnapshot snapshot;
  if (repository == nullptr) {
    return snapshot;
  }

  qtrade::config::v1::EngineConfig engine;
  std::uint64_t version = 0;
  const auto load_rc = repository->Load(scope, engine, version);
  if (load_rc == ErrorCode::kNotFound) {
    snapshot.set_version(1);
    snapshot.mutable_engine()->set_engine_id(scope.engine_id);
    return snapshot;
  }
  if (load_rc != ErrorCode::kSuccess) {
    spdlog::warn("[ConfigRepository] query scope tenant={} engine={} failed", scope.tenant_id, scope.engine_id);
    return snapshot;
  }

  snapshot.set_version(version);
  *snapshot.mutable_engine() = std::move(engine);
  if (snapshot.engine().engine_id().empty()) {
    snapshot.mutable_engine()->set_engine_id(scope.engine_id);
  }
  return snapshot;
}

}  // namespace qtrade::service
