/// @file      account_risk_client.hpp
/// @brief     引擎到账户硬风控服务的 E 段 gRPC 客户端
/// @details   供 OrderPipeline 在 OMS 落单前 Reserve，在拒单/撤单/成交后 Release
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_
#define QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/account_risk/v1/account_risk.pb.h>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace qtrade::client {

/// @brief AccountRiskClient 初始化选项
struct AccountRiskClientOptions {
  /// account-risk-service 地址，格式 host:port
  std::string server_address;
  /// 租户 ID
  std::string tenant_id;
  /// 交易账户号
  std::string account_id;
  /// 引擎实例 ID
  std::string engine_id;
  /// RPC 截止时间（毫秒）
  int timeout_ms = 3;
};

/// @brief 账户硬风控同步客户端
class AccountRiskClient {
 public:
  /// @brief 构造未初始化的客户端
  AccountRiskClient();

  /// @brief 析构并 Shutdown
  ~AccountRiskClient();

  /// @brief 按选项建立 gRPC 通道与 stub
  /// @param options 连接与账户上下文
  /// @return ErrorCode::kSuccess 表示成功；参数非法或重复 Init 返回 ErrorCode::kInternal
  ErrorCode Init(const AccountRiskClientOptions& options);

  /// @brief 释放通道与 stub
  void Shutdown();

  /// @brief 是否已完成 Init
  /// @return true 表示 stub 可用
  [[nodiscard]] bool IsInitialized() const;

  /// @brief 预占账户风控额度
  /// @param order_id 全局订单 ID
  /// @param request 策略下单请求
  /// @param risk_config_version 风控策略版本号
  /// @param response 服务端预占响应输出
  /// @return ErrorCode::kSuccess 表示 RPC 成功；未初始化或超时返回对应错误码
  ErrorCode ReserveOrder(const std::string& order_id,
                         const qtrade_sdk::trader::OrderRequest& request,
                         std::uint64_t risk_config_version,
                         qtrade::account_risk::v1::ReserveOrderResponse& response);

  /// @brief 释放已预占额度
  /// @param order_id 全局订单 ID
  /// @param reason Release 原因枚举整数值
  /// @param response 服务端释放响应输出
  /// @return ErrorCode::kSuccess 表示 RPC 成功；未初始化或超时返回对应错误码
  ErrorCode ReleaseOrder(const std::string& order_id,
                         int reason,
                         qtrade::account_risk::v1::ReleaseOrderResponse& response);

 private:
  /// 实现细节
  struct Impl;
  /// Pimpl
  std::unique_ptr<Impl> impl_;
};

}  // namespace qtrade::client

#endif  // QTRADE_CLIENT_ACCOUNT_RISK_CLIENT_HPP_
