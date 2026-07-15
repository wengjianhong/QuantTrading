/// @file      grpc_config.hpp
/// @brief     JSON "grpc" 段配置结构
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_CONFIG_GRPC_CONFIG_HPP_
#define QTRADE_COMMON_CONFIG_GRPC_CONFIG_HPP_

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace qtrade::common::config {

/// @brief gRPC 监听配置（对应 JSON "grpc"）
struct GrpcConfig {
  /// 监听主机
  std::string host = "0.0.0.0";
  /// 监听端口；0 表示未配置
  int port = 0;

  /// @brief 格式化为 host:port
  /// @return 监听地址字符串，示例："0.0.0.0:50051"
  [[nodiscard]] std::string ListenAddress() const;
};

/// @brief 从 JSON 对象解析 "grpc" 段
/// @param root 配置根对象或含 grpc 键的对象
/// @return 解析结果；无 grpc 段时返回 nullopt
[[nodiscard]] std::optional<GrpcConfig> ParseGrpcConfig(const nlohmann::json& root);

}  // namespace qtrade::common::config

#endif  // QTRADE_COMMON_CONFIG_GRPC_CONFIG_HPP_
