/// @file      account_risk_service.hpp
/// @brief     账户硬风控支撑服务生命周期封装
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_RISK_SERVICE_HPP_
#define QTRADE_SERVICE_ACCOUNT_RISK_SERVICE_HPP_

#include "qtrade/framework/support/support_sync_service_impl.hpp"
#include "qtrade/service/account_risk_service/grpc/account_risk_grpc_service.hpp"

namespace qtrade::service {

/// @brief 账户硬风控同步支撑服务
class AccountRiskService final : public qtrade::common::support::SupportSyncServiceImpl<AccountRiskGrpcService> {
 public:
  /// @brief 构造服务（默认监听端口 50060）
  AccountRiskService();

  /// @brief 加载 L0 配置并初始化数据库连接与表结构
  /// @param config_path 本地 JSON 配置路径
  /// @return ErrorCode::kSuccess 表示成功；配置或数据库失败返回对应错误码
  ErrorCode Initialize(const std::string& config_path) override;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_RISK_SERVICE_HPP_
