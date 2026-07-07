/// @file      registry_client.cpp
/// @brief     服务注册客户端实现
/// @details   实现服务注册与发现，对接 etcd 注册中心
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "client/registry_client/registry_client.hpp"

namespace qtrade::client {

ErrorCode RegistryClient::Init(std::string_view etcd_endpoints) {
  (void)etcd_endpoints;  // 预留参数：etcd服务地址列表
  initialized_ = true;
  return ErrorCode::kSuccess;
}

void RegistryClient::Shutdown() { initialized_ = false; }

ErrorCode RegistryClient::Register(std::string_view service_name,
                                   std::string_view instance_id,
                                   std::string_view address) {
  (void)service_name;  // 预留参数：服务名称
  (void)instance_id;   // 预留参数：实例ID
  (void)address;       // 预留参数：服务地址

  // 检查初始化状态
  if (!initialized_) {
    return ErrorCode::kNotInitialized;
  }

  // 预留实现：向etcd注册服务实例
  return ErrorCode::kSuccess;
}

}  // namespace qtrade::client
