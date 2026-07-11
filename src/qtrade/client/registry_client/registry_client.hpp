/// @file      registry_client.hpp
/// @brief     服务注册客户端
/// @details   提供服务注册与发现功能，对接 etcd 服务注册中心
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_REGISTRY_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_REGISTRY_CLIENT_HPP_
#include <qtrade/error_code/error_codes.hpp>

#include <string_view>

namespace qtrade::client {

/// @brief 服务注册客户端类
/// @details 封装服务注册与发现功能，对接 etcd 服务注册中心
class RegistryClient {
 public:
  /// @brief 初始化注册客户端
  /// @param etcd_endpoints etcd 服务端点地址列表
  /// @return ErrorCode::kSuccess 表示成功，其他表示失败
  ErrorCode Init(std::string_view etcd_endpoints);

  /// @brief 关闭注册客户端
  void Shutdown();

  /// @brief 注册服务实例
  /// @param service_name 服务名称
  /// @param instance_id 实例ID
  /// @param address 服务地址
  /// @return ErrorCode::kSuccess 表示成功，其他表示失败
  ErrorCode Register(std::string_view service_name, std::string_view instance_id, std::string_view address);

 private:
  bool initialized_ = false;  /// 初始化标志
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_REGISTRY_CLIENT_HPP_
