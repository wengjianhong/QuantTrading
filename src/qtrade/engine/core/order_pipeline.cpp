/// @file      order_pipeline.cpp
/// @brief     OrderPipeline 发单准入编排实现
/// @details   实现审计门禁、CMS/Risk 校验、E 段预占、OMS 落单与 EMS 入队及失败释放
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/order_pipeline.hpp"

#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/client/log_client/log_client.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.pb.h>

namespace qtrade::engine {

OrderPipeline::OrderPipeline(cms::ComplianceManager& compliance,
                             risk::RiskManager& risk_manager,
                             oms::OrderManager& order_manager,
                             ems::ExecutionManager& execution_manager,
                             qtrade::client::AccountRiskClient* account_risk_client)
  : compliance_(compliance),
    risk_manager_(risk_manager),
    order_manager_(order_manager),
    execution_manager_(execution_manager),
    account_risk_client_(account_risk_client) {}

void OrderPipeline::SetAccountRiskClient(qtrade::client::AccountRiskClient* account_risk_client) {
  account_risk_client_ = account_risk_client;
}

void OrderPipeline::SetLogClient(qtrade::client::LogClient* log_client) {
  log_client_ = log_client;
}

void OrderPipeline::SetReleaseHandler(ReleaseHandler handler) {
  release_handler_ = std::move(handler);
}

ErrorCode OrderPipeline::Submit(const qtrade_sdk::trader::OrderRequest& request) {
  // 1. 审计门禁、合规与实例风控
  if (log_client_ != nullptr && log_client_->IsAuditHalted()) {
    return ErrorCode::kInternalError;
  }
  if (const auto rc = compliance_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (const auto rc = risk_manager_.CheckOrder(request); rc != ErrorCode::kSuccess) {
    return rc;
  }
  if (request.client_order_id != 0 && order_manager_.GetOrderByClientId(request.client_order_id).has_value()) {
    return ErrorCode::kSuccess;
  }

  // 2. 分配订单 ID 并做 E 段预占
  const std::string order_id = order_manager_.AllocateOrderId();
  if (account_risk_client_ != nullptr) {
    qtrade::account_risk::v1::ReserveOrderResponse response;
    const auto reserve_result =
      account_risk_client_->ReserveOrder(order_id, request, risk_manager_.Version(), response);
    const bool reserve_unknown = reserve_result == ErrorCode::kTimeout ||
                                 (reserve_result == ErrorCode::kSuccess &&
                                  response.decision() == qtrade::account_risk::v1::ReserveOrderResponse::UNKNOWN);
    if (reserve_unknown) {
      qtrade::account_risk::v1::Reservation reservation;
      const auto query_result = account_risk_client_->GetReservation(order_id, reservation);
      if (query_result != ErrorCode::kSuccess || reservation.status() != "reserved") {
        return query_result == ErrorCode::kNotFound ? ErrorCode::kTimeout : query_result;
      }
    } else if (reserve_result != ErrorCode::kSuccess ||
               response.decision() != qtrade::account_risk::v1::ReserveOrderResponse::APPROVED) {
      return reserve_result == ErrorCode::kSuccess ? ErrorCode::kInternalError : reserve_result;
    }
  }

  // 3. OMS 落单；失败则释放预占
  const auto order = order_manager_.CreateOrder(request, order_id);
  if (!order.has_value()) {
    if (release_handler_) {
      (void)release_handler_(order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
    }
    return ErrorCode::kNotInitialized;
  }
  const std::string& persisted_order_id = order->order_id;
  const auto lifecycle = order_manager_.GetLifecycleState(persisted_order_id);
  if (lifecycle.has_value() && *lifecycle != oms::OrderLifecycleState::kPrepared) {
    return ErrorCode::kSuccess;
  }

  // 4. 持久化 EMS 入队事实后再交给执行线程
  if (const auto rc = order_manager_.MarkEmsQueued(persisted_order_id); rc != ErrorCode::kSuccess) {
    if (release_handler_) {
      (void)release_handler_(persisted_order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
    }
    return rc;
  }

  const auto rc = execution_manager_.Enqueue(*order);
  if (rc != ErrorCode::kSuccess) {
    (void)order_manager_.RecordSendResult(persisted_order_id, rc);
    if (release_handler_) {
      (void)release_handler_(persisted_order_id, qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
    }
  }
  return rc;
}

}  // namespace qtrade::engine
