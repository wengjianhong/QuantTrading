/// @file      account_risk_grpc_service.hpp
/// @brief     AccountRiskService 同步 gRPC 实现声明
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_RISK_GRPC_SERVICE_HPP_
#define QTRADE_SERVICE_ACCOUNT_RISK_GRPC_SERVICE_HPP_

#include "qtrade/dao/dao_manager.hpp"
#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.grpc.pb.h>

#include <memory>

namespace qtrade::service {

/// @brief 账户硬风控 gRPC 服务实现（MVP：进程内账簿）
class AccountRiskGrpcService final : public qtrade::account_risk::v1::AccountRiskService::Service {
 public:
  /// @brief 构造 gRPC 服务
  /// @param connection 数据库连接池管理器；当前预占仍主要走内存账簿
  /// @param dao 本进程 DaoManager（表结构已在启动时确保；持久化路径后续使用）
  AccountRiskGrpcService(std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection,
                         std::shared_ptr<qtrade::framework::dao::DaoManager> dao);

  /// @brief 预占账户额度
  /// @param context gRPC 上下文
  /// @param request 预占请求
  /// @param response 预占响应
  /// @return gRPC 状态
  grpc::Status ReserveOrder(grpc::ServerContext* context,
                            const qtrade::account_risk::v1::ReserveOrderRequest* request,
                            qtrade::account_risk::v1::ReserveOrderResponse* response) override;

  /// @brief 查询指定订单的预占状态
  /// @param context gRPC 上下文
  /// @param request 查询请求
  /// @param response 预占状态
  /// @return 找到返回 OK，不存在返回 NOT_FOUND
  grpc::Status GetReservation(grpc::ServerContext* context,
                              const qtrade::account_risk::v1::GetReservationRequest* request,
                              qtrade::account_risk::v1::GetReservationResponse* response) override;

  /// @brief 释放账户预占
  /// @param context gRPC 上下文
  /// @param request 释放请求
  /// @param response 释放响应
  /// @return gRPC 状态
  grpc::Status ReleaseOrder(grpc::ServerContext* context,
                            const qtrade::account_risk::v1::ReleaseOrderRequest* request,
                            qtrade::account_risk::v1::ReleaseOrderResponse* response) override;

  /// @brief 列出指定账户上的有效预占
  /// @param context gRPC 上下文
  /// @param request 列表请求
  /// @param response 列表响应
  /// @return gRPC 状态
  grpc::Status ListActiveReservations(grpc::ServerContext* context,
                                      const qtrade::account_risk::v1::ListActiveReservationsRequest* request,
                                      qtrade::account_risk::v1::ListActiveReservationsResponse* response) override;

  /// @brief 查询账户风控策略
  /// @param context gRPC 上下文
  /// @param request 查询请求
  /// @param response 策略响应
  /// @return gRPC 状态
  grpc::Status GetAccountRiskPolicy(grpc::ServerContext* context,
                                    const qtrade::account_risk::v1::GetAccountRiskPolicyRequest* request,
                                    qtrade::account_risk::v1::GetAccountRiskPolicyResponse* response) override;

  /// @brief 写入或更新账户风控策略
  /// @param context gRPC 上下文
  /// @param request 含策略的写入请求
  /// @param response 空响应
  /// @return gRPC 状态
  grpc::Status UpsertAccountRiskPolicy(grpc::ServerContext* context,
                                       const qtrade::account_risk::v1::UpsertAccountRiskPolicyRequest* request,
                                       qtrade::account_risk::v1::UpsertAccountRiskPolicyResponse* response) override;

 private:
  /// 数据库连接池管理器
  std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection_pool_mgr_;
  /// 本进程 DaoManager
  std::shared_ptr<qtrade::framework::dao::DaoManager> dao_mgr_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_RISK_GRPC_SERVICE_HPP_
