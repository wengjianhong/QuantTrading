#include "qtrade/common/config/qtrade_log_service_config.hpp"

#include "qtrade/common/json/json_util.hpp"
namespace qtrade::common::config {
std::optional<QtradeLogServiceConfig> ParseQtradeLogServiceConfig(const std::string& text) {
  const auto root = ParseJsonString(text);
  if (!root.has_value()) {
    return std::nullopt;
  }
  const auto& root_json = root.value();
  const auto grpc = ParseGrpcConfig(root_json);
  if (!grpc.has_value() || !root_json.contains("storage") || !root_json.contains("ingest")) {
    return std::nullopt;
  }

  const auto& storage = root_json.at("storage");
  const auto& ingest = root_json.at("ingest");
  QtradeLogServiceConfig out;
  out.grpc = *grpc;
  out.storage_type = storage.value("type", "");
  out.storage_path = storage.value("path", "");
  out.retention_days = storage.value("retention_days", 0);
  out.batch_size = ingest.value("batch_size", 0);
  out.flush_interval_ms = ingest.value("flush_interval_ms", 0);
  return out.storage_type.empty() || out.retention_days < 1 || out.batch_size < 1 || out.flush_interval_ms < 1
           ? std::nullopt
           : std::optional<QtradeLogServiceConfig>(out);
}
}  // namespace qtrade::common::config
