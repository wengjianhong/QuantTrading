/// @file      account_client.hpp
/// @brief     account-service gRPC 客户端（薄封装）
/// @details   方法名与 proto AccountService 中引擎使用的 RPC 对应。
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_ACCOUNT_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_ACCOUNT_CLIENT_HPP_

#include "qtrade/common/config/service_config.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/account/v1/account.pb.h>

#include <memory>

namespace qtrade::client {

/// @brief AccountClient 初始化选项
struct AccountClientOptions {
  /// account-service 连出端点
  qtrade::common::config::ServiceConfig service_config;
};

/// @brief AccountService gRPC 客户端
class AccountClient {
 public:
  AccountClient();
  ~AccountClient();
  AccountClient(const AccountClient&) = delete;
  AccountClient& operator=(const AccountClient&) = delete;

  /// @brief 建立 gRPC 通道与 stub
  ErrorCode Init(const AccountClientOptions& options);

  /// @brief 是否已 Init
  [[nodiscard]] bool IsInitialized() const;

  /// @brief 释放通道与 stub
  void Shutdown();

  /// ======================== RPC 业务接口实现 ========================

  /// @brief 获取交易账户凭证
  /// @param request 获取交易账户凭证请求
  /// @param response 获取交易账户凭证响应
  /// @return ErrorCode::kSuccess 表示成功；未初始化返回 kNotInitialized；RPC 失败返回 kTimeout
  ErrorCode GetCredential(const qtrade::account::v1::GetCredentialRequest& request,
                          qtrade::account::v1::GetCredentialResponse& response);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_ACCOUNT_CLIENT_HPP_
