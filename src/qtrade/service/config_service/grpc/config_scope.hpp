/// @file      config_scope.hpp
/// @brief     配置作用域与 gRPC 快照组装
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_SCOPE_HPP_
#define QTRADE_SERVICE_CONFIG_SCOPE_HPP_

#include <qtrade/proto/config/v1/config.pb.h>

#include <string>

namespace qtrade::service {

/// @brief 配置作用域（DB 主键：tenant_id + engine_id；gRPC 请求仅传 engine_id）
struct ConfigScope {
  /// 租户 ID
  std::string tenant_id = "default";
  /// 引擎实例 ID
  std::string engine_id = "default";

  friend auto operator<=>(const ConfigScope&, const ConfigScope&) = default;
};

/// @brief 从 GetConfig 请求构造作用域
[[nodiscard]] ConfigScope MakeConfigScope(const qtrade::config::v1::GetConfigRequest& request);

/// @brief 从 SubscribeConfig 请求构造作用域
[[nodiscard]] ConfigScope MakeConfigScope(const qtrade::config::v1::SubscribeConfigRequest& request);

/// @brief 查库并组装 ConfigSnapshot（gRPC 响应）
[[nodiscard]] qtrade::config::v1::ConfigSnapshot QueryConfigSnapshot(const ConfigScope& scope);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_SCOPE_HPP_
