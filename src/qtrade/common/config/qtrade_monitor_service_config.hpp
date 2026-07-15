/// @file      qtrade_monitor_service_config.hpp
/// @brief     监控服务 L0 引导配置声明。
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_MONITOR_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_MONITOR_SERVICE_CONFIG_HPP_
#include "qtrade/common/config/grpc_config.hpp"

#include <optional>
#include <string>
namespace qtrade::common::config {
/// @brief 监控采集、留存和告警配置。
struct QtradeMonitorServiceConfig {
  /// gRPC 监听配置
  GrpcConfig grpc;
  /// 抓取周期（秒）
  int scrape_interval_sec = 0;
  /// 指标保留天数
  int retention_days = 0;
  /// 是否启用告警
  bool alert_enabled = false;
  /// 告警 Webhook 地址
  std::string alert_webhook;
};
/// @brief 解析监控服务本地 JSON 配置。
/// @param json JSON 文本。
/// @return 配置合法时返回强类型配置，否则返回 std::nullopt。
[[nodiscard]] std::optional<QtradeMonitorServiceConfig> ParseQtradeMonitorServiceConfig(const std::string& json);
}  // namespace qtrade::common::config
#endif
