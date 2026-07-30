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

/// @brief 策略插件引导项（JSON：config.strategy）
struct QtradeEngineStrategyBootstrap {
  /// 策略插件 .so 目录
  std::string plugin_dir;
};

/// @brief 引擎进程本地配置（JSON：config）
struct QtradeEngineProcessConfig {
  /// 日志目录
  std::string log_dir = "logs";
  /// 日志文件名
  std::string log_filename = "qtrade_engine.log";
  /// 策略插件
  QtradeEngineStrategyBootstrap strategy;
  /// 实例身份
  QtradeEngineIdentity identity;
};

/// @brief 引擎依赖的支撑服务端点
struct QtradeEngineSupportServices {
  /// config-service
  ServiceConfig config_service;
  /// account-service
  ServiceConfig account_service;
  /// account-risk-service；enabled=false 时不启用 E 段
  ServiceConfig account_risk_service;
};

/// @brief 对应 config/qtrade_engine.json
/// @details config 含日志/策略插件/身份；support_services 为带 name 的端点数组。
///          业务/适配器/策略清单由 config-service 下发。
struct QtradeEngineBootstrapConfig {
  /// 进程本地配置
  QtradeEngineProcessConfig config;
  /// 支撑服务连出配置
  QtradeEngineSupportServices support_services;
};

/// @brief 从引擎进程配置 JSON 对象解析
/// @param config_node 形如 { "config", "support_services" } 的对象
/// @return 解析结果；非对象或必填段缺失时返回 nullopt
[[nodiscard]] std::optional<QtradeEngineBootstrapConfig> ParseQtradeEngineBootstrapConfig(
  const nlohmann::json& config_node);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_QTRADE_ENGINE_BOOTSTRAP_CONFIG_HPP_
