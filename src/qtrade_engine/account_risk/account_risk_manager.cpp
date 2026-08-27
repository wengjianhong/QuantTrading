/// @file      account_risk_manager.cpp
/// @brief     账户硬风控管理器实现
/// @author    wengjianhong
/// @date      2026-08-14
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/account_risk/account_risk_manager.hpp"

#include <spdlog/spdlog.h>

#include <optional>

namespace qtrade::engine::account_risk {
namespace {

/// @brief 启动对账：本地无单或已终态则释放预占
/// @param state OMS 生命周期；空表示柜台/OMS 均无该单
/// @return 应释放时返回原因；应保留时返回 nullopt
[[nodiscard]] std::optional<qtrade::account_risk::ReleaseReason> ReleaseReasonIfStale(
  std::optional<orders::OrderLifecycleState> state) {
  if (!state.has_value()) {
    return qtrade::account_risk::ReleaseReason::kExpired;
  }
  switch (*state) {
    case orders::OrderLifecycleState::kFilled:
      return qtrade::account_risk::ReleaseReason::kSettled;
    case orders::OrderLifecycleState::kCanceled:
      return qtrade::account_risk::ReleaseReason::kCanceled;
    case orders::OrderLifecycleState::kRejected:
      return qtrade::account_risk::ReleaseReason::kRejectedByVenue;
    default:
      return std::nullopt;
  }
}

}  // namespace

AccountRiskManager::AccountRiskManager() = default;

AccountRiskManager::~AccountRiskManager() {
  Stop();
}

void AccountRiskManager::SetBridge(qtrade::account_risk::IAccountRiskBridge* bridge) {
  std::lock_guard lock(mutex_);
  bridge_ = bridge;
}

void AccountRiskManager::SetIdentity(std::string account_id, std::string engine_id) {
  std::lock_guard lock(mutex_);
  account_id_ = std::move(account_id);
  engine_id_ = std::move(engine_id);
}

ErrorCode AccountRiskManager::CheckActiveReservations(const orders::OrderApi& orders) {
  qtrade::account_risk::IAccountRiskBridge* bridge = nullptr;
  std::string account_id;
  {
    std::lock_guard lock(mutex_);
    bridge = bridge_;
    account_id = account_id_;
  }
  if (bridge == nullptr) {
    return ErrorCode::kSuccess;
  }
  if (account_id.empty()) {
    return ErrorCode::kSystemError;
  }

  const auto listed = bridge->ListActiveReservations(account_id);
  if (listed.error_code != ErrorCode::kSuccess || !listed.data.has_value()) {
    spdlog::error("ListActiveReservations failed, code={}", static_cast<int>(listed.error_code));
    return listed.error_code == ErrorCode::kSuccess ? ErrorCode::kInternalError : listed.error_code;
  }

  for (const auto& reservation : *listed.data) {
    if (reservation.order_id.empty() || reservation.state != qtrade::account_risk::ReservationState::kReserved) {
      continue;
    }
    const auto reason = ReleaseReasonIfStale(orders.GetLifecycleState(reservation.order_id));
    if (!reason.has_value()) {
      continue;
    }
    spdlog::info(
      "reconcile release reservation order_id={} reason={}", reservation.order_id, static_cast<int>(*reason));
    InvokeRelease(bridge, account_id, ReleaseItem{reservation.order_id, *reason});
  }
  return ErrorCode::kSuccess;
}

void AccountRiskManager::Start() {
  std::lock_guard lock(mutex_);
  if (running_) {
    return;
  }
  stopping_ = false;
  running_ = true;
  worker_ = std::thread([this] { Run(); });
}

void AccountRiskManager::Stop() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
    if (!running_ && !worker_.joinable()) {
      queue_.clear();
      return;
    }
    running_ = false;
  }
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
}

ErrorCode AccountRiskManager::Reserve(const qtrade::sdk::trader::OrderRequest& request, const std::string& order_id) {
  // 1. 在锁内取得本次桥接调用所需的稳定配置快照。
  qtrade::account_risk::IAccountRiskBridge* bridge = nullptr;
  std::string account_id;
  std::string engine_id;
  {
    std::lock_guard lock(mutex_);
    bridge = bridge_;
    account_id = account_id_;
    engine_id = engine_id_;
  }
  if (bridge == nullptr) {
    return ErrorCode::kSuccess;
  }

  // 2. 将本地下单请求映射为账户风控预占请求。
  qtrade::account_risk::ReserveRequest reserve_request;
  reserve_request.account_id = account_id;
  reserve_request.order_id = order_id;
  reserve_request.exposure.engine_id = engine_id;
  reserve_request.exposure.strategy_id = request.strategy_id;
  reserve_request.exposure.instrument_id = request.instrument;
  reserve_request.exposure.price = request.price;
  reserve_request.exposure.quantity = static_cast<std::uint64_t>(request.volume);
  reserve_request.exposure.notional = request.price * static_cast<double>(request.volume);
  reserve_request.exposure.side = request.side;
  reserve_request.expected_policy_version = 0;

  // 3. 预占结果未知时使用相同 order_id 查询确认，避免重复预占。
  const auto reserve_result = bridge->Reserve(reserve_request);
  const bool reserve_unknown = reserve_result.error_code == ErrorCode::kTimeout ||
                               (reserve_result.error_code == ErrorCode::kSuccess && reserve_result.data.has_value() &&
                                reserve_result.data->state == qtrade::account_risk::ReservationState::kUnspecified);
  if (reserve_unknown) {
    const auto query_result = bridge->QueryReservation(account_id, order_id);
    if (query_result.error_code != ErrorCode::kSuccess || !query_result.data.has_value() ||
        query_result.data->state != qtrade::account_risk::ReservationState::kReserved) {
      return query_result.error_code == ErrorCode::kNotFound ? ErrorCode::kTimeout : query_result.error_code;
    }
    return ErrorCode::kSuccess;
  }
  if (reserve_result.error_code != ErrorCode::kSuccess || !reserve_result.data.has_value() ||
      reserve_result.data->state != qtrade::account_risk::ReservationState::kReserved) {
    return reserve_result.error_code == ErrorCode::kSuccess ? ErrorCode::kInternalError : reserve_result.error_code;
  }
  return ErrorCode::kSuccess;
}

void AccountRiskManager::Release(std::string order_id, qtrade::account_risk::ReleaseReason reason) {
  {
    std::unique_lock lock(mutex_);
    if (bridge_ == nullptr || order_id.empty()) {
      return;
    }
    cv_.wait(lock, [this] { return stopping_ || queue_.size() < kQueueCapacity; });
    if (stopping_) {
      return;
    }
    queue_.push_back(ReleaseItem{std::move(order_id), reason});
  }
  cv_.notify_one();
}

void AccountRiskManager::Run() {
  while (true) {
    ReleaseItem item;
    qtrade::account_risk::IAccountRiskBridge* bridge = nullptr;
    std::string account_id;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return !running_ || !queue_.empty(); });
      if (!running_ && queue_.empty()) {
        return;
      }
      item = std::move(queue_.front());
      queue_.pop_front();
      bridge = bridge_;
      account_id = account_id_;
    }
    InvokeRelease(bridge, account_id, item);
  }
}

void AccountRiskManager::InvokeRelease(qtrade::account_risk::IAccountRiskBridge* bridge,
                                       const std::string& account_id,
                                       const ReleaseItem& item) {
  if (bridge == nullptr || item.order_id.empty()) {
    return;
  }
  qtrade::account_risk::ReleaseRequest request;
  request.account_id = account_id;
  request.order_id = item.order_id;
  request.reason = item.reason;
  const auto result = bridge->Release(request);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("Release failed: order_id={}, code={}", item.order_id, static_cast<int>(result.error_code));
  }
}

}  // namespace qtrade::engine::account_risk
