/// @file      config_client.hpp
/// @brief     config-service gRPC 客户端（薄封装）
/// @details   方法名与 proto ConfigService RPC 一一对应；不负责业务配置应用。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_

#include "qtrade/common/config/service_config.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.pb.h>

#include <functional>
#include <memory>

namespace qtrade::client {

/// @brief ConfigClient 初始化选项
struct ConfigClientOptions {
  /// config-service 连出端点
  qtrade::common::config::ServiceConfig service_config;
};

/// @brief ConfigService gRPC 客户端
class ConfigClient {
 public:
  /// @brief SubscribeEngineConfig 每条推送的回调
  using SubscribeHandler = std::function<void(const qtrade::config::v1::SubscribeEngineConfigResponse& response)>;

  ConfigClient();
  ~ConfigClient();
  ConfigClient(const ConfigClient&) = delete;
  ConfigClient& operator=(const ConfigClient&) = delete;

  /// @brief 建立 gRPC 通道与 stub
  ErrorCode Init(const ConfigClientOptions& options);

  /// @brief 是否已 Init
  [[nodiscard]] bool IsInitialized() const;

  /// @brief 停止订阅线程并释放资源
  void Shutdown();

  /// ======================== RPC 业务接口实现 ========================
  /// @brief 获取引擎配置
  /// @param request 获取引擎配置请求
  /// @param response 获取引擎配置响应
  /// @return ErrorCode::kSuccess 表示成功；未初始化返回 kNotInitialized；RPC 失败返回 kTimeout
  ErrorCode GetEngineConfig(const qtrade::config::v1::GetEngineConfigRequest& request,
                            qtrade::config::v1::GetEngineConfigResponse& response);

  /// @brief 订阅引擎配置变更
  /// @details 断线后按已收到的最大 version 自动续订；on_message 在订阅线程回调
  ///
  /// @param request 订阅引擎配置变更请求
  /// @param on_message 订阅回调
  /// @return ErrorCode::kSuccess 表示成功；未初始化返回 kNotInitialized；RPC 失败返回 kTimeout
  ErrorCode SubscribeEngineConfig(const qtrade::config::v1::SubscribeEngineConfigRequest& request,
                                  SubscribeHandler on_message);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_
