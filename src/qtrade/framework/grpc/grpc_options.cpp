/// @file      grpc_options.cpp
/// @brief     GrpcOptions / ParseGrpcOptions 实现
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/grpc/grpc_options.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

namespace qtrade::common {

std::string GrpcOptions::ListenAddress() const {
  return host + ":" + std::to_string(port);
}

GrpcOptions ParseGrpcOptions(const std::string& json_path, const int default_port) {
  GrpcOptions options;
  options.port = default_port;

  // 1. 打开配置文件；失败则保留默认端口
  std::ifstream ifs(json_path);
  if (!ifs.is_open()) {
    return options;
  }

  // 2. 解析 JSON；异常则打日志并返回默认
  nlohmann::json root;
  try {
    ifs >> root;
  } catch (const nlohmann::json::exception& ex) {
    spdlog::warn("[GrpcOptions] invalid JSON in {}: {}", json_path, ex.what());
    return options;
  }

  // 3. 读取 grpc.host / grpc.port（若存在）
  if (!root.contains("grpc") || !root["grpc"].is_object()) {
    return options;
  }

  const auto& grpc = root["grpc"];
  if (grpc.contains("host")) {
    options.host = grpc["host"].get<std::string>();
  }
  if (grpc.contains("port")) {
    options.port = grpc["port"].get<int>();
  }
  return options;
}

}  // namespace qtrade::common
