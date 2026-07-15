/// @file      grpc_options.hpp
/// @brief     解析 JSON 配置中的 gRPC 段（各支撑服务共用）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_GRPC_OPTIONS_HPP_
#define QTRADE_COMMON_GRPC_GRPC_OPTIONS_HPP_

#include <string>

namespace qtrade::common {

/// @brief gRPC 服务端选项（对应 JSON 中的 "grpc" 段）
struct GrpcOptions {
  std::string host = "0.0.0.0";  ///< 监听主机；默认 0.0.0.0
  int port = 0;                  ///< 监听端口；0 表示未配置，解析时可用默认端口填入

  /// @brief 格式化为 gRPC 监听地址
  /// @return 形如 "0.0.0.0:50051" 的地址字符串
  [[nodiscard]] std::string ListenAddress() const;
};

/// @brief 从 JSON 配置文件解析 "grpc" 段
/// @param json_path 配置文件路径
/// @param default_port 未配置 port 时使用的默认端口
/// @return 解析结果；文件缺失或无效 JSON 时返回带 default_port 的默认选项
[[nodiscard]] GrpcOptions ParseGrpcOptions(const std::string& json_path, int default_port);

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_GRPC_GRPC_OPTIONS_HPP_
