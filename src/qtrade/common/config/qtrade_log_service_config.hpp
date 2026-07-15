/// @file      qtrade_log_service_config.hpp
/// @brief     qtrade_log_service.json 配置结构
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_LOG_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_LOG_SERVICE_CONFIG_HPP_

#include "qtrade/common/config/grpc_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 对应 config/qtrade_log_service.json
struct QtradeLogServiceConfig {
  /// gRPC 监听
  GrpcConfig grpc;
  /// 存储后端类型
  std::string storage_type;
  /// 存储路径
  std::string storage_path;
  /// 日志保留天数
  int retention_days = 0;
  /// 单批写入条数
  int batch_size = 0;
  /// 刷盘间隔（毫秒）
  int flush_interval_ms = 0;
};

/// @brief 从 JSON 字符串解析日志服务配置
/// @param json JSON 文本
/// @return 解析结果；JSON 非法或必填字段无效时返回 nullopt
[[nodiscard]] std::optional<QtradeLogServiceConfig> ParseQtradeLogServiceConfig(const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_LOG_SERVICE_CONFIG_HPP_
