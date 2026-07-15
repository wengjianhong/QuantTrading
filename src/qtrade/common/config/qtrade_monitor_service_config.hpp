/// @file      qtrade_monitor_service_config.hpp
/// @brief     qtrade_monitor_service.json 配置结构
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_MONITOR_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_MONITOR_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/grpc_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 对应 config/qtrade_monitor_service.json
struct QtradeMonitorServiceConfig {
  /// gRPC 监听
  GrpcConfig grpc;
  /// 指标抓取周期（秒）
  int scrape_interval_sec = 0;
  /// 指标保留天数
  int retention_days = 0;
  /// 是否启用告警
  bool alert_enabled = false;
  /// 告警 Webhook 地址
  std::string alert_webhook;
};

/// @brief 从 JSON 字符串解析监控服务配置
/// @param json JSON 文本
/// @return 解析结果；JSON 非法或必填字段无效时返回 nullopt
[[nodiscard]] std::optional<QtradeMonitorServiceConfig> ParseQtradeMonitorServiceConfig(const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_MONITOR_SERVICE_CONFIG_HPP_
