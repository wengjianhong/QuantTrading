#include "qtrade/common/config/qtrade_monitor_service_config.hpp"

#include "qtrade/common/json/json_util.hpp"
namespace qtrade::common::config {
std::optional<QtradeMonitorServiceConfig> ParseQtradeMonitorServiceConfig(const std::string& text) {
  const auto root = ParseJsonString(text);
  if (!root.has_value()) {
    return std::nullopt;
  }
  const auto& root_json = root.value();
  const auto grpc = ParseGrpcConfig(root_json);
  if (!grpc.has_value() || !root_json.contains("metrics") || !root_json.contains("alert")) {
    return std::nullopt;
  }

  const auto& metrics = root_json.at("metrics");
  const auto& alert = root_json.at("alert");
  QtradeMonitorServiceConfig out;
  out.grpc = *grpc;
  out.scrape_interval_sec = metrics.value("scrape_interval_sec", 0);
  out.retention_days = metrics.value("retention_days", 0);
  out.alert_enabled = alert.value("enabled", false);
  out.alert_webhook = alert.value("webhook", "");
  return out.scrape_interval_sec > 0 && out.retention_days > 0 ? std::optional<QtradeMonitorServiceConfig>(out)
                                                               : std::nullopt;
}
}  // namespace qtrade::common::config
