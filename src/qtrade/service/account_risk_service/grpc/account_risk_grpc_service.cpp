/// @file      account_risk_grpc_service.cpp
/// @brief     按账户串行的账户硬风控预占实现（MVP 内存账簿）
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_risk_service/grpc/account_risk_grpc_service.hpp"

#include "qtrade/common/system/time.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace qtrade::service {
namespace {

using AccountRiskPolicy = qtrade::account_risk::v1::AccountRiskPolicy;
using Reservation = qtrade::account_risk::v1::Reservation;

/// @brief 单笔预占状态
struct ReservationState {
  /// 预占 ID
  std::string reservation_id;
  /// 状态字符串（reserved / released / expired 等）
  std::string status = "reserved";
  /// 预占名义金额
  double notional = 0.0;
  /// 预占保证金
  double margin = 0.0;
  /// 过期时间（Unix 毫秒）
  std::int64_t expires_at_unix_ms = 0;
};

/// @brief 单账户内存账簿
struct LedgerState {
  /// 账户风控策略
  AccountRiskPolicy policy;
  /// 已预占名义金额合计
  double reserved_notional = 0.0;
  /// 已预占保证金合计
  double reserved_margin = 0.0;
  /// 已预占未完成订单数
  std::uint64_t reserved_open_orders = 0;
  /// order_id → 预占状态
  std::unordered_map<std::string, ReservationState> reservations;
};

/// 保护全局账簿
std::mutex g_ledger_mutex;
/// tenant+account → 账簿
std::unordered_map<std::string, LedgerState> g_ledgers;

/// @brief 生成账簿主键
/// @param tenant_id 租户 ID
/// @param account_id 账户 ID
/// @return 组合键
std::string AccountKey(const std::string& tenant_id, const std::string& account_id) {
  return tenant_id + "\n" + account_id;
}

/// @brief 将已过期预占从账簿占用中扣除
/// @param ledger 账户账簿
/// @param now 当前 Unix 毫秒
void ExpireReservations(LedgerState& ledger, std::int64_t now) {
  for (auto& [_, reservation] : ledger.reservations) {
    if (reservation.status == "reserved" && reservation.expires_at_unix_ms <= now) {
      reservation.status = "expired";
      ledger.reserved_notional = std::max(0.0, ledger.reserved_notional - reservation.notional);
      ledger.reserved_margin = std::max(0.0, ledger.reserved_margin - reservation.margin);
      if (ledger.reserved_open_orders > 0) {
        --ledger.reserved_open_orders;
      }
    }
  }
}

/// @brief 构造 INVALID_ARGUMENT 状态
/// @param message 错误说明
/// @return gRPC 状态
grpc::Status InvalidArgument(const std::string& message) {
  return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, message);
}

}  // namespace

AccountRiskGrpcService::AccountRiskGrpcService(
  std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection,
  std::shared_ptr<qtrade::framework::dao::DaoManager> dao)
  : connection_pool_mgr_(std::move(connection)), dao_mgr_(std::move(dao)) {
  (void)dao_mgr_;
}

grpc::Status AccountRiskGrpcService::ReserveOrder(grpc::ServerContext*,
                                                  const qtrade::account_risk::v1::ReserveOrderRequest* request,
                                                  qtrade::account_risk::v1::ReserveOrderResponse* response) {
  if (request->tenant_id().empty() || request->account_id().empty() || request->intent().order_id().empty()) {
    return InvalidArgument("tenant_id, account_id and intent.order_id are required");
  }

  const std::int64_t now = qtrade::common::system::UnixMillisNow();
  std::lock_guard lock(g_ledger_mutex);
  auto ledger_it = g_ledgers.find(AccountKey(request->tenant_id(), request->account_id()));
  if (ledger_it == g_ledgers.end()) {
    response->set_decision(qtrade::account_risk::v1::ReserveOrderResponse::REJECTED);
    response->set_reject_reason("ACCOUNT_RISK_POLICY_NOT_FOUND");
    return grpc::Status::OK;
  }
  LedgerState& ledger = ledger_it->second;
  ExpireReservations(ledger, now);

  const AccountRiskPolicy& policy = ledger.policy;
  response->set_policy_version(policy.version());
  if (!policy.enabled() || (policy.valid_until_unix_ms() > 0 && now > policy.valid_until_unix_ms())) {
    response->set_decision(qtrade::account_risk::v1::ReserveOrderResponse::REJECTED);
    response->set_reject_reason(policy.enabled() ? "ACCOUNT_RISK_POLICY_EXPIRED" : "ACCOUNT_RISK_DISABLED");
    return grpc::Status::OK;
  }
  if (request->risk_config_version() != 0 && request->risk_config_version() != policy.version()) {
    response->set_decision(qtrade::account_risk::v1::ReserveOrderResponse::REJECTED);
    response->set_reject_reason("RISK_POLICY_VERSION_MISMATCH");
    return grpc::Status::OK;
  }

  const std::string& order_id = request->intent().order_id();
  if (const auto existing = ledger.reservations.find(order_id); existing != ledger.reservations.end()) {
    response->set_decision(existing->second.status == "reserved"
                             ? qtrade::account_risk::v1::ReserveOrderResponse::APPROVED
                             : qtrade::account_risk::v1::ReserveOrderResponse::REJECTED);
    response->set_reservation_id(existing->second.reservation_id);
    response->set_expires_at_unix_ms(existing->second.expires_at_unix_ms);
    if (existing->second.status != "reserved") {
      response->set_reject_reason("ORDER_ID_ALREADY_TERMINAL");
    }
    return grpc::Status::OK;
  }

  const double notional = request->intent().estimated_notional() > 0.0
                            ? request->intent().estimated_notional()
                            : request->intent().price() * static_cast<double>(request->intent().quantity());
  const double margin = request->intent().estimated_margin();
  const bool notional_exceeded =
    policy.max_notional() > 0.0 && ledger.reserved_notional + notional > policy.max_notional() - policy.safety_buffer();
  const bool margin_exceeded =
    policy.max_margin() > 0.0 && ledger.reserved_margin + margin > policy.max_margin() - policy.safety_buffer();
  const bool orders_exceeded =
    policy.max_open_orders() > 0 && ledger.reserved_open_orders + 1 > policy.max_open_orders();
  if (notional_exceeded || margin_exceeded || orders_exceeded) {
    response->set_decision(qtrade::account_risk::v1::ReserveOrderResponse::REJECTED);
    response->set_reject_reason(notional_exceeded ? "MAX_NOTIONAL_EXCEEDED"
                                : margin_exceeded ? "MAX_MARGIN_EXCEEDED"
                                                  : "MAX_OPEN_ORDERS_EXCEEDED");
    return grpc::Status::OK;
  }

  const std::int64_t ttl_ms = request->reservation_ttl_ms() > 0 ? request->reservation_ttl_ms() : 300000;
  ReservationState reservation;
  reservation.reservation_id = "RSV-" + order_id;
  reservation.notional = notional;
  reservation.margin = margin;
  reservation.expires_at_unix_ms = now + ttl_ms;
  ledger.reserved_notional += notional;
  ledger.reserved_margin += margin;
  ++ledger.reserved_open_orders;
  ledger.reservations.emplace(order_id, reservation);

  response->set_decision(qtrade::account_risk::v1::ReserveOrderResponse::APPROVED);
  response->set_reservation_id(reservation.reservation_id);
  response->set_expires_at_unix_ms(reservation.expires_at_unix_ms);
  return grpc::Status::OK;
}

grpc::Status AccountRiskGrpcService::GetReservation(grpc::ServerContext*,
                                                    const qtrade::account_risk::v1::GetReservationRequest* request,
                                                    qtrade::account_risk::v1::GetReservationResponse* response) {
  if (request->tenant_id().empty() || request->account_id().empty() || request->order_id().empty()) {
    return InvalidArgument("tenant_id, account_id and order_id are required");
  }

  std::lock_guard lock(g_ledger_mutex);
  const auto ledger_it = g_ledgers.find(AccountKey(request->tenant_id(), request->account_id()));
  if (ledger_it == g_ledgers.end()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "account risk ledger not found");
  }
  ExpireReservations(ledger_it->second, qtrade::common::system::UnixMillisNow());
  const auto reservation_it = ledger_it->second.reservations.find(request->order_id());
  if (reservation_it == ledger_it->second.reservations.end()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "reservation not found");
  }

  Reservation* reservation = response->mutable_reservation();
  reservation->set_order_id(request->order_id());
  reservation->set_reservation_id(reservation_it->second.reservation_id);
  reservation->set_status(reservation_it->second.status);
  reservation->set_expires_at_unix_ms(reservation_it->second.expires_at_unix_ms);
  return grpc::Status::OK;
}

grpc::Status AccountRiskGrpcService::ReleaseOrder(grpc::ServerContext*,
                                                  const qtrade::account_risk::v1::ReleaseOrderRequest* request,
                                                  qtrade::account_risk::v1::ReleaseOrderResponse* response) {
  if (request->tenant_id().empty() || request->account_id().empty() || request->order_id().empty()) {
    return InvalidArgument("tenant_id, account_id and order_id are required");
  }
  std::lock_guard lock(g_ledger_mutex);
  const auto ledger_it = g_ledgers.find(AccountKey(request->tenant_id(), request->account_id()));
  if (ledger_it == g_ledgers.end()) {
    response->set_released(false);
    response->set_reject_reason("ACCOUNT_RISK_POLICY_NOT_FOUND");
    return grpc::Status::OK;
  }
  LedgerState& ledger = ledger_it->second;
  const auto reservation_it = ledger.reservations.find(request->order_id());
  if (reservation_it == ledger.reservations.end()) {
    response->set_released(true);
    return grpc::Status::OK;
  }
  ReservationState& reservation = reservation_it->second;
  if (reservation.status == "reserved") {
    reservation.status =
      request->reason() == qtrade::account_risk::v1::ReleaseOrderRequest::SETTLED ? "settled" : "released";
    ledger.reserved_notional = std::max(0.0, ledger.reserved_notional - reservation.notional);
    ledger.reserved_margin = std::max(0.0, ledger.reserved_margin - reservation.margin);
    if (ledger.reserved_open_orders > 0) {
      --ledger.reserved_open_orders;
    }
  }
  response->set_released(true);
  return grpc::Status::OK;
}

grpc::Status AccountRiskGrpcService::ListActiveReservations(
  grpc::ServerContext*,
  const qtrade::account_risk::v1::ListActiveReservationsRequest* request,
  qtrade::account_risk::v1::ListActiveReservationsResponse* response) {
  std::lock_guard lock(g_ledger_mutex);
  const auto ledger_it = g_ledgers.find(AccountKey(request->tenant_id(), request->account_id()));
  if (ledger_it == g_ledgers.end()) {
    return grpc::Status::OK;
  }
  ExpireReservations(ledger_it->second, qtrade::common::system::UnixMillisNow());
  for (const auto& [order_id, state] : ledger_it->second.reservations) {
    if (state.status != "reserved") {
      continue;
    }
    Reservation* out = response->add_reservations();
    out->set_order_id(order_id);
    out->set_reservation_id(state.reservation_id);
    out->set_status(state.status);
    out->set_expires_at_unix_ms(state.expires_at_unix_ms);
  }
  return grpc::Status::OK;
}

grpc::Status AccountRiskGrpcService::GetAccountRiskPolicy(
  grpc::ServerContext*,
  const qtrade::account_risk::v1::GetAccountRiskPolicyRequest* request,
  qtrade::account_risk::v1::GetAccountRiskPolicyResponse* response) {
  std::lock_guard lock(g_ledger_mutex);
  const auto ledger_it = g_ledgers.find(AccountKey(request->tenant_id(), request->account_id()));
  if (ledger_it == g_ledgers.end()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "account risk policy not found");
  }
  *response->mutable_policy() = ledger_it->second.policy;
  return grpc::Status::OK;
}

grpc::Status AccountRiskGrpcService::UpsertAccountRiskPolicy(
  grpc::ServerContext*,
  const qtrade::account_risk::v1::UpsertAccountRiskPolicyRequest* request,
  qtrade::account_risk::v1::UpsertAccountRiskPolicyResponse*) {
  const AccountRiskPolicy& policy = request->policy();
  if (policy.tenant_id().empty() || policy.account_id().empty() || policy.version() == 0) {
    return InvalidArgument("tenant_id, account_id and non-zero version are required");
  }
  std::lock_guard lock(g_ledger_mutex);
  LedgerState& ledger = g_ledgers[AccountKey(policy.tenant_id(), policy.account_id())];
  if (ledger.policy.version() > policy.version()) {
    return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "risk policy version cannot decrease");
  }
  ledger.policy = policy;
  return grpc::Status::OK;
}

}  // namespace qtrade::service
