/// @file      qtrade_log_service_config.hpp
/// @brief     日志服务 L0 引导配置声明。
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_LOG_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_LOG_SERVICE_CONFIG_HPP_
#include "qtrade/common/config/grpc_config.hpp"

#include <optional>
#include <string>
namespace qtrade::common::config {
/// @brief 日志服务本地监听、存储和批量写入配置。
struct QtradeLogServiceConfig {
  /// gRPC 监听配置
  GrpcConfig grpc;
  /// 存储后端类型
  std::string storage_type;
  /// 存储路径
  std::string storage_path;
  /// 保留天数
  int retention_days = 0;
  /// 单批写入条数
  int batch_size = 0;
  /// 刷盘间隔（毫秒）
  int flush_interval_ms = 0;
};
/// @brief 解析日志服务本地 JSON 配置。
/// @param json JSON 文本。
/// @return 配置合法时返回强类型配置，否则返回 std::nullopt。
[[nodiscard]] std::optional<QtradeLogServiceConfig> ParseQtradeLogServiceConfig(const std::string& json);
}  // namespace qtrade::common::config
#endif
