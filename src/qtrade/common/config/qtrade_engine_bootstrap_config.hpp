/// @file      qtrade_engine_bootstrap_config.hpp
/// @brief     qtrade_engine.json 进程引导配置
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_QTRADE_ENGINE_BOOTSTRAP_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_QTRADE_ENGINE_BOOTSTRAP_CONFIG_HPP_

#include "qtrade/common/config/service_config.hpp"

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 引擎实例身份
struct QtradeEngineIdentity {
  /// 租户 ID
  std::string tenant_id = "default";
  /// 引擎实例 ID
  std::string engine_id = "default";
  /// 交易账户号
  std::string account_id;
};

/// @brief 引擎依赖的支撑服务端点
struct QtradeEngineSupportServices {
  /// config-service
  ServiceConfig config_service;
  /// account-service
  ServiceConfig account_service;
  /// account-risk-service；enabled=false 时不启用 E 段
  ServiceConfig account_risk_service;
  /// log-service；extensions 可含 topic
  ServiceConfig log_service;
};

/// @brief 对应 config/qtrade_engine.json
/// @details 含实例身份、支撑服务端点与策略插件目录。业务/适配器/策略清单由 config-service 下发。
struct QtradeEngineBootstrapConfig {
  /// 实例身份
  QtradeEngineIdentity identity;
  /// 支撑服务连出配置
  QtradeEngineSupportServices support_services;
  /// 策略插件 .so 目录（启动时扫描加载）
  std::string strategy_plugin_dir;
};

/// @brief 从引擎进程配置 JSON 对象解析
/// @param config_node 形如 { "identity", "support_services" } 的对象
/// @return 解析结果；非对象或必填段缺失时返回 nullopt
[[nodiscard]] std::optional<QtradeEngineBootstrapConfig> ParseQtradeEngineBootstrapConfig(
  const nlohmann::json& config_node);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ENGINE_BOOTSTRAP_CONFIG_HPP_
