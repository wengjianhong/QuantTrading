/// @file      qtrade_engine_config.hpp
/// @brief     qtrade_engine.json 进程引导配置
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_ENGINE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_ENGINE_CONFIG_HPP_

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 对应 config/qtrade_engine.json
/// @details 业务配置由 config-service 下发；凭证经 account-service GetCredential 拉取
struct QtradeEngineConfig {
  /// config-service gRPC 地址；空则跳过
  std::string config_service;
  /// account-service gRPC 地址；空则跳过
  std::string account_service;
  /// account-risk-service gRPC 地址；账户硬风控启用时必填
  std::string account_risk_service;
  /// 是否启用账户级 E 段硬风控
  bool account_risk_enabled = false;
  /// E 段 RPC 截止时间（毫秒）
  int account_risk_timeout_ms = 3;
  /// 租户 ID
  std::string tenant_id = "default";
  /// 引擎实例 ID
  std::string engine_id = "default";
  /// 交易账户号
  std::string account_id;
  /// config-service 不可用时加载的只读 L1 快照路径
  std::string fallback_engine_config_path;
  /// 日志服务地址；MVP 本地 sink 可为空
  std::string log_service;
  /// log_client 主题
  std::string log_topic = "engine";
  /// monitor 端点；空则 stub://local
  std::string monitor_endpoint;
};

/// @brief 从 JSON 字符串解析引擎进程配置
/// @param json JSON 文本
/// @return 解析结果；JSON 非法时返回 nullopt
[[nodiscard]] std::optional<QtradeEngineConfig> ParseQtradeEngineConfig(const std::string& json);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ENGINE_CONFIG_HPP_
