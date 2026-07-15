/// @file      engine_options.hpp
/// @brief     交易引擎启动选项（对齐 qtrade_engine.json / QtradeEngineConfig）
/// @author    wengjianhong
/// @date      2026-06-22
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_OPTIONS_HPP_
#define QTRADE_ENGINE_ENGINE_OPTIONS_HPP_

#include "qtrade/common/config/qtrade_engine_config.hpp"

namespace qtrade::engine {

/// @brief 交易引擎进程引导选项
struct EngineOptions {
  std::string config_server_address;   ///< config-service gRPC 地址；空则跳过 config_client
  std::string account_server_address;  ///< account-service gRPC 地址；空则跳过 account_client
  std::string tenant_id = "default";   ///< 租户 ID（与交易账户主键一致）
  std::string engine_id = "default";   ///< 引擎实例 ID
  std::string account_id;              ///< 交易账户号（GetCredential 入参，租户内唯一）
  std::string log_topic = "engine";    ///< log_client 日志主题
  std::string monitor_endpoint;        ///< monitor_client 端点；空则使用 stub://local

  /// @brief 从 QtradeEngineConfig 填充
  /// @param config common/config 解析结果
  /// @return 引擎选项
  [[nodiscard]] static EngineOptions FromConfig(const qtrade::common::config::QtradeEngineConfig& config) {
    EngineOptions options;
    options.config_server_address = config.config_service;
    options.account_server_address = config.account_service;
    options.tenant_id = config.tenant_id;
    options.engine_id = config.engine_id;
    options.account_id = config.account_id;
    options.log_topic = config.log_topic;
    options.monitor_endpoint = config.monitor_endpoint;
    return options;
  }
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_OPTIONS_HPP_
