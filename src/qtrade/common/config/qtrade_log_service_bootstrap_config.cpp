/// @file      qtrade_log_service_bootstrap_config.cpp
/// @brief     QtradeLogServiceBootstrapConfig 解析实现
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_log_service_bootstrap_config.hpp"

namespace qtrade::common::config {

std::optional<QtradeLogServiceBootstrapConfig> ParseQtradeLogServiceBootstrapConfig(
  const nlohmann::json& config_node) {
  if (!config_node.is_object()) {
    return std::nullopt;
  }
  if (!config_node.contains("grpc") || !config_node.at("grpc").is_object()) {
    return std::nullopt;
  }
  const auto grpc = ParseServiceEndpoint(config_node.at("grpc"));
  if (!grpc.has_value() || !config_node.contains("storage") || !config_node.contains("ingest")) {
    return std::nullopt;
  }

  const auto& storage = config_node.at("storage");
  const auto& ingest = config_node.at("ingest");
  QtradeLogServiceBootstrapConfig out;
  out.grpc = grpc.value();
  out.storage_type = storage.value("type", "");
  out.storage_path = storage.value("path", "");
  out.retention_days = storage.value("retention_days", 0);
  out.batch_size = ingest.value("batch_size", 0);
  out.flush_interval_ms = ingest.value("flush_interval_ms", 0);
  if (out.storage_type.empty() || out.retention_days < 1 || out.batch_size < 1 || out.flush_interval_ms < 1) {
    return std::nullopt;
  }
  return out;
}

}  // namespace qtrade::common::config
