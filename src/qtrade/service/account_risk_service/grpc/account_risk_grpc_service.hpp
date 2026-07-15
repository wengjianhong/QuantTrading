/// @file account_risk_grpc_service.hpp
/// @brief AccountRiskService 同步 gRPC 实现
#ifndef QTRADE_SERVICE_ACCOUNT_RISK_GRPC_SERVICE_HPP_
#define QTRADE_SERVICE_ACCOUNT_RISK_GRPC_SERVICE_HPP_

#include "qtrade/framework/database/db_connection.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.grpc.pb.h>

#include <memory>

namespace qtrade::service {

class AccountRiskGrpcService final : public qtrade::account_risk::v1::AccountRiskService::Service {
 public:
  explicit AccountRiskGrpcService(std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection);

  grpc::Status ReserveOrder(grpc::ServerContext* context,
                            const qtrade::account_risk::v1::ReserveOrderRequest* request,
                            qtrade::account_risk::v1::ReserveOrderResponse* response) override;
  grpc::Status ReleaseOrder(grpc::ServerContext* context,
                            const qtrade::account_risk::v1::ReleaseOrderRequest* request,
                            qtrade::account_risk::v1::ReleaseOrderResponse* response) override;
  grpc::Status ListActiveReservations(grpc::ServerContext* context,
                                      const qtrade::account_risk::v1::ListActiveReservationsRequest* request,
                                      qtrade::account_risk::v1::ListActiveReservationsResponse* response) override;
  grpc::Status GetAccountRiskPolicy(grpc::ServerContext* context,
                                    const qtrade::account_risk::v1::GetAccountRiskPolicyRequest* request,
                                    qtrade::account_risk::v1::AccountRiskPolicy* response) override;
  grpc::Status UpsertAccountRiskPolicy(grpc::ServerContext* context,
                                       const qtrade::account_risk::v1::UpsertAccountRiskPolicyRequest* request,
                                       qtrade::account_risk::v1::Empty* response) override;

 private:
  std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_RISK_GRPC_SERVICE_HPP_
