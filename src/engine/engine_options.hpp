/// @file      engine_options.hpp
/// @brief     交易引擎启动选项
/// @author    wengjianhong
/// @date      2026-06-22
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_OPTIONS_HPP_
#define QTRADE_ENGINE_ENGINE_OPTIONS_HPP_

#include <string>

namespace qtrade::engine {

/// @brief 交易引擎进程引导选项（本地 JSON 仅含出站连接身份）
/// @details 业务配置（account、quote、strategies）由 config-service 下发 EngineConfig，见 config.proto
struct EngineOptions {
  std::string config_server_address;   ///< config-service gRPC 地址；空则跳过 config_client
  std::string account_server_address;  ///< account-service gRPC 地址；空则跳过 account_client
  std::string tenant_id = "default";   ///< 租户 ID（与 GetConfigRequest / EngineConfig 对齐）
  std::string engine_id = "default";   ///< 引擎实例 ID
  std::string log_topic = "engine";    ///< log_client 日志主题
  std::string monitor_endpoint;        ///< monitor_client 端点；空则使用 stub://local
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_OPTIONS_HPP_
