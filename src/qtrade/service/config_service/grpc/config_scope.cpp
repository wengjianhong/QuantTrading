/// @file      config_scope.cpp
/// @brief     配置作用域与 gRPC 快照组装实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/config_service/grpc/config_scope.hpp"

#include "qtrade/common/converter/proto_json_converter.hpp"
#include "qtrade/dao/config_service/engine/engine_config.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::service {
using qtrade::common::converter::ConvertJsonToProto;
namespace {

std::string NormalizeScopeField(const std::string& value) {
  return value.empty() ? "default" : value;
}

}  // namespace

ConfigScope MakeConfigScope(const qtrade::config::v1::GetEngineConfigRequest& request) {
  return ConfigScope{
    .tenant_id = "default",
    .engine_id = NormalizeScopeField(request.engine_id()),
  };
}

ConfigScope MakeConfigScope(const qtrade::config::v1::SubscribeEngineConfigRequest& request) {
  return ConfigScope{
    .tenant_id = "default",
    .engine_id = NormalizeScopeField(request.engine_id()),
  };
}

qtrade::config::v1::EngineConfig QueryEngineConfig(const ConfigScope& scope,
                                                   qtrade::framework::dao::EngineConfig& engine_config_dao,
                                                   cpputils::database::IConnection& connection) {
  qtrade::config::v1::EngineConfig config;

  qtrade::framework::dao::EngineConfigRecord where;
  where.tenant_id = scope.tenant_id;
  where.engine_id = scope.engine_id;

  const auto result = engine_config_dao.Select(connection, where);
  if (result.error_code == ErrorCode::kNotFound ||
      (result.error_code == ErrorCode::kSuccess && (!result.data.has_value() || result.data->empty()))) {
    config.set_version(1);
    config.set_engine_id(scope.engine_id);
    return config;
  }
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("[ConfigScope] query scope tenant={} engine={} failed", scope.tenant_id, scope.engine_id);
    return config;
  }

  const auto& record = result.data->front();
  qtrade::config::v1::EngineConfig engine;
  if (!record.payload.has_value() || !ConvertJsonToProto(record.payload.value(), engine, {}, "ConfigScope")) {
    spdlog::error("[ConfigScope] invalid payload JSON for tenant={} engine={}", scope.tenant_id, scope.engine_id);
    return config;
  }

  engine.set_version(record.version.value_or(0));
  if (engine.engine_id().empty()) {
    engine.set_engine_id(scope.engine_id);
  }
  return engine;
}

}  // namespace qtrade::service
