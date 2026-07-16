/// @file      config_scope.cpp
/// @brief     配置作用域与 gRPC 快照组装实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/grpc/config_scope.hpp"

#include "qtrade/common/proto/proto_json_converter.hpp"
#include "qtrade/dao/config_service/engine/engine_config.hpp"

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

qtrade::config::v1::ConfigSnapshot QueryConfigSnapshot(const ConfigScope& scope) {
  qtrade::config::v1::ConfigSnapshot snapshot;

  qtrade::framework::dao::EngineConfigRecord where;
  where.tenant_id = scope.tenant_id;
  where.engine_id = scope.engine_id;

  const auto result = qtrade::framework::dao::EngineConfig::Instance().Select(where);
  if (result.error_code == ErrorCode::kNotFound ||
      (result.error_code == ErrorCode::kSuccess && (!result.data.has_value() || result.data->empty()))) {
    snapshot.set_version(1);
    snapshot.mutable_engine()->set_engine_id(scope.engine_id);
    return snapshot;
  }
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("[ConfigScope] query scope tenant={} engine={} failed", scope.tenant_id, scope.engine_id);
    return snapshot;
  }

  const auto& record = result.data->front();
  qtrade::config::v1::EngineConfig engine;
  if (!record.payload.has_value() ||
      !qtrade::common::ProtoFromJson(record.payload.value(), engine, {}, "ConfigScope")) {
    spdlog::error("[ConfigScope] invalid payload JSON for tenant={} engine={}", scope.tenant_id, scope.engine_id);
    return snapshot;
  }

  snapshot.set_version(record.version.value_or(0));
  *snapshot.mutable_engine() = std::move(engine);
  if (snapshot.engine().engine_id().empty()) {
    snapshot.mutable_engine()->set_engine_id(scope.engine_id);
  }
  return snapshot;
}

}  // namespace qtrade::service
