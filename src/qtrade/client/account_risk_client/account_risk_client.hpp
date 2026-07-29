/// @file      account_risk_client.hpp
/// @brief     account-risk-service gRPC 客户端（薄封装）
/// @details   方法名与 proto AccountRiskService 中引擎使用的 RPC 对应。
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_
#define QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_

#include "qtrade/common/config/service_config.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/account_risk/v1/account_risk.pb.h>

#include <memory>

namespace qtrade::client {

/// @brief AccountRiskClient 初始化选项
struct AccountRiskClientOptions {
  /// account-risk-service 连出端点（timeout_ms<=0 时默认 3ms）
  qtrade::common::config::ServiceConfig service_config;
};

/// @brief AccountRiskService gRPC 客户端
class AccountRiskClient {
 public:
  AccountRiskClient();
  ~AccountRiskClient();
  AccountRiskClient(const AccountRiskClient&) = delete;
  AccountRiskClient& operator=(const AccountRiskClient&) = delete;

  /// @brief 建立 gRPC 通道与 stub
  ErrorCode Init(const AccountRiskClientOptions& options);

  /// @brief 是否已 Init
  [[nodiscard]] bool IsInitialized() const;

  /// @brief 释放通道与 stub
  void Shutdown();

  /// ======================== RPC 业务接口实现 ========================
  /// @brief 预占订单
  /// @param request 预占订单请求
  /// @param response 预占订单响应
  /// @return ErrorCode::kSuccess 表示成功；未初始化返回 kNotInitialized；RPC 失败返回 kTimeout
  ErrorCode ReserveOrder(const qtrade::account_risk::v1::ReserveOrderRequest& request,
                         qtrade::account_risk::v1::ReserveOrderResponse& response);

  /// @brief 查询订单预占状态
  /// @param request 查询订单预占状态请求
  /// @param response 查询订单预占状态响应
  /// @return ErrorCode::kSuccess 表示成功；未初始化返回 kNotInitialized；RPC 失败返回 kTimeout
  ErrorCode GetReservation(const qtrade::account_risk::v1::GetReservationRequest& request,
                           qtrade::account_risk::v1::GetReservationResponse& response);

  /// @brief 释放订单预占
  /// @param request 释放订单预占请求
  /// @param response 释放订单预占响应
  /// @return ErrorCode::kSuccess 表示成功；未初始化返回 kNotInitialized；RPC 失败返回 kTimeout
  ErrorCode ReleaseOrder(const qtrade::account_risk::v1::ReleaseOrderRequest& request,
                         qtrade::account_risk::v1::ReleaseOrderResponse& response);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qtrade::client

#endif  // QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_
