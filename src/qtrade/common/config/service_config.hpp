/// @file      service_config.hpp
/// @brief     通用服务端点配置（监听或连出）
/// @details   固定字段 host/port/enabled/timeout_ms；其余键进入 extensions（如 topic）
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_SERVICE_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_SERVICE_CONFIG_HPP_

#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief 服务端点配置
/// @details 可用于 JSON `grpc` 监听段，或引擎 `support_services.*` 连出端点
struct ServiceConfig {
  /// 主机；监听时多为 0.0.0.0，连出时为对端地址
  std::string host = "0.0.0.0";
  /// 端口；0 表示未配置
  int port = 0;
  /// 是否启用该端点；默认 true
  bool enabled = true;
  /// RPC 截止时间（毫秒）；0 表示未配置/使用调用方默认
  int timeout_ms = 0;
  /// 扩展字段（非固定键），值统一为字符串；如 log 的 topic
  std::map<std::string, std::string> extensions;

  /// @brief 格式化为 host:port
  /// @return 地址字符串，示例："127.0.0.1:50051"
  [[nodiscard]] std::string Address() const;

  /// @brief 同 Address（兼容旧 ListenAddress 调用）
  /// @return 地址字符串
  [[nodiscard]] std::string ListenAddress() const {
    return Address();
  }

  /// @brief 读取扩展字段
  /// @param key 扩展键名
  /// @return 存在时返回值，否则 nullopt
  [[nodiscard]] std::optional<std::string> Extension(const std::string& key) const;

  /// @brief 端点是否已配置有效 host/port
  /// @return true 表示可连接或可监听
  [[nodiscard]] bool IsConfigured() const {
    return !host.empty() && port > 0;
  }
};

/// @brief 从端点 JSON 对象解析
/// @param endpoint 形如 { "host", "port", "enabled", "timeout_ms", ... } 的对象
/// @return 解析结果；非对象或 host/port 非法时返回 nullopt
[[nodiscard]] std::optional<ServiceConfig> ParseServiceEndpoint(const nlohmann::json& endpoint);

/// @brief 从配置根对象解析 "grpc" 监听段
/// @param root 配置根对象
/// @return 解析结果；无 grpc 段时返回 nullopt
[[nodiscard]] std::optional<ServiceConfig> ParseServiceConfig(const nlohmann::json& root);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_SERVICE_CONFIG_HPP_
