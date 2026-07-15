/// @file account_risk_service.hpp
/// @brief 账户硬风控支撑服务
#ifndef QTRADE_SERVICE_ACCOUNT_RISK_SERVICE_HPP_
#define QTRADE_SERVICE_ACCOUNT_RISK_SERVICE_HPP_

#include "qtrade/framework/support/support_sync_service_impl.hpp"
#include "qtrade/service/account_risk_service/grpc/account_risk_grpc_service.hpp"

namespace qtrade::service {

class AccountRiskService final : public qtrade::common::support::SupportSyncServiceImpl<AccountRiskGrpcService> {
 public:
  AccountRiskService();
  ErrorCode Initialize(const std::string& config_path) override;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_RISK_SERVICE_HPP_
