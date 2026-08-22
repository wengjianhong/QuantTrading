/// @file      order_manager.cpp
/// @brief     订单管理器实现（进程内内存状态机；含 OrderApi）
/// @details   分区与头文件一致：生命周期 → OrderApi → 本地查询 → 柜台对接 → 内部辅助。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/orders/order_manager.hpp"

#include "qtrade/common/utils/trade_dedup.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace qtrade::engine::orders {
namespace trader = qtrade::sdk::trader;
using qtrade::common::utils::GenerateTradeDedupKey;

namespace {

/// @brief 是否为终态生命周期（Filled / Canceled / Rejected）
/// @param state 订单生命周期状态
/// @return 终态返回 true
bool IsTerminalLifecycle(OrderLifecycleState state) {
  return state == OrderLifecycleState::kFilled || state == OrderLifecycleState::kCanceled ||
         state == OrderLifecycleState::kRejected;
}

}  // namespace

// =============================================================================
// 生命周期（组合根）
// =============================================================================

OrderManager::OrderManager() = default;

OrderManager::~OrderManager() {
  Shutdown();
}

ErrorCode OrderManager::Initialize(const OrderManagerOptions& options) {
  // 1. 校验身份字段
  if (options.account_id.empty() || options.engine_id.empty() || options.engine_epoch == 0) {
    return ErrorCode::kSystemError;
  }
  std::lock_guard lock(mutex_);
  if (initialized_) {
    return ErrorCode::kSystemError;
  }
  // 2. 绑定身份并清空内存表
  account_id_ = options.account_id;
  engine_id_ = options.engine_id;
  engine_epoch_ = options.engine_epoch;
  orders_.clear();
  client_order_index_.clear();
  applied_trade_ids_.clear();
  reconciled_order_ids_.clear();
  order_id_counter_.store(0, std::memory_order_release);
  initialized_ = true;
  spdlog::info("[OrderManager] initialized (in-memory, no journal)");
  return ErrorCode::kSuccess;
}

void OrderManager::Shutdown() {
  std::lock_guard lock(mutex_);
  orders_.clear();
  client_order_index_.clear();
  applied_trade_ids_.clear();
  reconciled_order_ids_.clear();
  initialized_ = false;
  spdlog::info("[OrderManager] shutdown");
}

// =============================================================================
// OrderApi：模块间稳定接口
// =============================================================================

std::string OrderManager::AllocateOrderId() {
  const auto sequence = order_id_counter_.fetch_add(1, std::memory_order_acq_rel) + 1;
  return account_id_ + "-" + engine_id_ + "-" + std::to_string(engine_epoch_) + "-" + std::to_string(sequence);
}

std::optional<trader::Order> OrderManager::CreateOrder(const trader::OrderRequest& request,
                                                       const std::string& order_id) {
  std::lock_guard lock(mutex_);
  if (!initialized_ || order_id.empty()) {
    return std::nullopt;
  }
  // 同 client_order_id 幂等：返回已有快照
  if (request.client_order_id != 0) {
    const auto existing = client_order_index_.find(request.client_order_id);
    if (existing != client_order_index_.end()) {
      return orders_.at(existing->second).order;
    }
  }

  // 写入本地条目，生命周期从 kPrepared 起
  OrderEntry entry;
  entry.order.order_id = order_id;
  entry.order.client_order_id = request.client_order_id;
  entry.order.instrument = request.instrument;
  entry.order.market = request.market;
  entry.order.price = request.price;
  entry.order.volume = request.volume;
  entry.order.left_volume = request.volume;
  entry.order.price_type = request.price_type;
  entry.order.side = request.side;
  entry.order.position_effect = request.position_effect;
  entry.order.business_type = request.business_type;
  entry.order.status = trader::OrderStatusType::kNew;
  entry.order.submit_status = trader::OrderSubmitStatusType::kInit;
  entry.lifecycle_state = OrderLifecycleState::kPrepared;

  orders_[order_id] = entry;
  if (request.client_order_id != 0) {
    client_order_index_[request.client_order_id] = order_id;
  }
  spdlog::info("[OrderManager] order created: {}", order_id);
  return entry.order;
}

std::optional<trader::Order> OrderManager::GetOrderByClientId(std::uint32_t client_order_id) const {
  std::lock_guard lock(mutex_);
  const auto index_it = client_order_index_.find(client_order_id);
  if (index_it == client_order_index_.end()) {
    return std::nullopt;
  }
  const auto order_it = orders_.find(index_it->second);
  return order_it == orders_.end() ? std::nullopt : std::optional<trader::Order>(order_it->second.order);
}

std::optional<trader::Order> OrderManager::GetOrder(const std::string& order_id) const {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  return it != orders_.end() ? std::optional<trader::Order>(it->second.order) : std::nullopt;
}

std::optional<OrderLifecycleState> OrderManager::GetLifecycleState(const std::string& order_id) const {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  return it == orders_.end() ? std::nullopt : std::optional<OrderLifecycleState>(it->second.lifecycle_state);
}

ErrorCode OrderManager::CancelOrder(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  return ApplyTransition(it->second, OrderLifecycleState::kCancelPending);
}

ErrorCode OrderManager::MarkEmsQueued(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  return ApplyTransition(it->second, OrderLifecycleState::kEmsQueued);
}

ErrorCode OrderManager::MarkSendPending(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  it->second.order.submit_status = trader::OrderSubmitStatusType::kInsertSubmitted;
  return ApplyTransition(it->second, OrderLifecycleState::kSendPending);
}

ErrorCode OrderManager::RecordSendResult(const std::string& order_id, ErrorCode result) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }

  // 成功 → Working；超时 → SendUnknown；其他失败 → Rejected
  auto target_state = it->second.lifecycle_state;
  if (target_state == OrderLifecycleState::kSendPending ||
      (target_state == OrderLifecycleState::kEmsQueued && result != ErrorCode::kSuccess)) {
    target_state = OrderLifecycleState::kWorking;
    if (result == ErrorCode::kTimeout) {
      target_state = OrderLifecycleState::kSendUnknown;
      it->second.order.status = trader::OrderStatusType::kUnknown;
    } else if (result != ErrorCode::kSuccess) {
      target_state = OrderLifecycleState::kRejected;
      it->second.order.status = trader::OrderStatusType::kRejected;
      it->second.order.submit_status = trader::OrderSubmitStatusType::kInsertRejected;
    }
  }
  return ApplyTransition(it->second, target_state);
}

ErrorCode OrderManager::RecordCancelResult(const std::string& order_id, ErrorCode result) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  if (result == ErrorCode::kSuccess || result == ErrorCode::kTimeout) {
    return ApplyTransition(it->second, OrderLifecycleState::kCancelPending);
  }

  it->second.order.submit_status = trader::OrderSubmitStatusType::kCancelRejected;
  const auto fallback =
    it->second.order.traded_volume > 0 ? OrderLifecycleState::kPartiallyFilled : OrderLifecycleState::kWorking;
  return ApplyTransition(it->second, fallback);
}

// =============================================================================
// 本地内存查询与敞口
// =============================================================================

std::optional<trader::Order> OrderManager::CreateOrder(const trader::OrderRequest& request) {
  return CreateOrder(request, AllocateOrderId());
}

std::vector<trader::Order> OrderManager::GetOrdersRequiringReconciliation() const {
  std::lock_guard lock(mutex_);
  std::vector<trader::Order> orders;
  for (const auto& [order_id, entry] : orders_) {
    (void)order_id;
    if (!reconciled_order_ids_.contains(entry.order.order_id) &&
        (entry.lifecycle_state == OrderLifecycleState::kSendPending ||
         entry.lifecycle_state == OrderLifecycleState::kSendUnknown ||
         entry.lifecycle_state == OrderLifecycleState::kWorking ||
         entry.lifecycle_state == OrderLifecycleState::kCancelPending)) {
      orders.push_back(entry.order);
    }
  }
  return orders;
}

void OrderManager::MarkReconciled(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  if (orders_.contains(order_id)) {
    reconciled_order_ids_.insert(order_id);
  }
}

std::uint64_t OrderManager::GetActiveOrderCount() const {
  std::lock_guard lock(mutex_);
  std::uint64_t count = 0;
  for (const auto& [order_id, entry] : orders_) {
    (void)order_id;
    if (!IsTerminalLifecycle(entry.lifecycle_state)) {
      ++count;
    }
  }
  return count;
}

double OrderManager::GetOpenNotional() const {
  std::lock_guard lock(mutex_);
  double notional = 0.0;
  for (const auto& [order_id, entry] : orders_) {
    (void)order_id;
    if (!IsTerminalLifecycle(entry.lifecycle_state)) {
      notional += entry.order.price * static_cast<double>(entry.order.left_volume);
    }
  }
  return notional;
}

// =============================================================================
// 柜台对接：回报与对账（不直接调用 TraderApi）
// =============================================================================

void OrderManager::ApplyOrderReport(const trader::Order& report) {
  std::lock_guard lock(mutex_);
  // 1. 定位本地订单（无本地则忽略，不 Adopt）
  auto it = FindOrderLocked(report.order_id, report.client_order_id);
  if (it == orders_.end()) {
    spdlog::warn("[OrderManager] ignored order report without local order, client_order_id={}", report.client_order_id);
    return;
  }

  // 2. 合并快照并按柜台状态投影生命周期
  OrderEntry updated = it->second;
  MergeOrderSnapshot(updated, report);
  const auto target = LifecycleFromOrderStatus(report.status, updated.lifecycle_state);
  if (ApplyTransition(updated, target) == ErrorCode::kSuccess) {
    it->second = std::move(updated);
  }
}

void OrderManager::ApplyTradeReport(const trader::Trade& report) {
  std::lock_guard lock(mutex_);
  // 1. 成交幂等
  const std::string dedup_key = GenerateTradeDedupKey(report);
  if (applied_trade_ids_.contains(dedup_key)) {
    return;
  }

  // 2. 定位本地订单
  auto it = FindOrderLocked(report.order_id, report.client_order_id);
  if (it == orders_.end()) {
    spdlog::warn("[OrderManager] ignored trade report without local order, client_order_id={}", report.client_order_id);
    return;
  }

  // 3. 累计成交并推进生命周期
  OrderEntry updated = it->second;
  trader::Order& local = updated.order;
  local.traded_volume += report.volume;
  local.left_volume = std::max<std::int64_t>(0, local.volume - local.traded_volume);
  local.trade_amount += report.trade_amount;
  local.status = local.left_volume == 0 ? trader::OrderStatusType::kFilled : trader::OrderStatusType::kPartiallyFilled;
  const auto target = local.left_volume == 0 ? OrderLifecycleState::kFilled : OrderLifecycleState::kPartiallyFilled;
  if (ApplyTransition(updated, target) == ErrorCode::kSuccess) {
    it->second = std::move(updated);
    applied_trade_ids_.insert(dedup_key);
  }
}

void OrderManager::ReconcileBrokerOrder(const trader::Order& report) {
  std::lock_guard lock(mutex_);
  if (!initialized_) {
    return;
  }

  auto it = FindOrderLocked(report.order_id, report.client_order_id);
  if (it == orders_.end()) {
    // 冷启动无本地 journal：按柜台快照重建内存条目，禁止触发补单
    std::string order_id = report.order_id;
    if (order_id.empty()) {
      if (report.client_order_id == 0) {
        spdlog::warn("[OrderManager] skip broker order without order_id/client_order_id");
        return;
      }
      order_id = AllocateOrderId();
    }
    OrderEntry entry;
    entry.order = report;
    entry.order.order_id = order_id;
    entry.lifecycle_state = LifecycleFromOrderStatus(report.status, OrderLifecycleState::kWorking);
    orders_[order_id] = entry;
    if (report.client_order_id != 0) {
      client_order_index_[report.client_order_id] = order_id;
    }
    reconciled_order_ids_.insert(order_id);
    spdlog::info("[OrderManager] adopted broker order into memory: {}", order_id);
    return;
  }

  OrderEntry updated = it->second;
  MergeOrderSnapshot(updated, report);
  const auto target = LifecycleFromOrderStatus(report.status, updated.lifecycle_state);
  if (ApplyTransition(updated, target) == ErrorCode::kSuccess) {
    it->second = std::move(updated);
    reconciled_order_ids_.insert(it->second.order.order_id);
  }
}

// =============================================================================
// 内部：本地查找
// =============================================================================

std::unordered_map<std::string, OrderManager::OrderEntry>::iterator OrderManager::FindOrderLocked(
  const std::string& order_id, std::uint32_t client_order_id) {
  auto it = orders_.find(order_id);
  if (it == orders_.end() && client_order_id != 0) {
    const auto index_it = client_order_index_.find(client_order_id);
    if (index_it != client_order_index_.end()) {
      it = orders_.find(index_it->second);
    }
  }
  return it;
}

std::unordered_map<std::string, OrderManager::OrderEntry>::const_iterator OrderManager::FindOrderLocked(
  const std::string& order_id, std::uint32_t client_order_id) const {
  auto it = orders_.find(order_id);
  if (it == orders_.end() && client_order_id != 0) {
    const auto index_it = client_order_index_.find(client_order_id);
    if (index_it != client_order_index_.end()) {
      it = orders_.find(index_it->second);
    }
  }
  return it;
}

// =============================================================================
// 内部：业务状态机
// =============================================================================

ErrorCode OrderManager::ApplyTransition(OrderEntry& entry, OrderLifecycleState target_state) {
  if (!CanTransition(entry.lifecycle_state, target_state)) {
    spdlog::error("[OrderManager] invalid state transition: order={}, from={}, to={}",
                  entry.order.order_id,
                  static_cast<int>(entry.lifecycle_state),
                  static_cast<int>(target_state));
    return ErrorCode::kSystemError;
  }
  entry.lifecycle_state = target_state;
  return ErrorCode::kSuccess;
}

bool OrderManager::CanTransition(OrderLifecycleState from, OrderLifecycleState to) {
  if (from == to) {
    return true;
  }
  switch (from) {
    case OrderLifecycleState::kPrepared:
      return to == OrderLifecycleState::kEmsQueued || to == OrderLifecycleState::kRejected ||
             to == OrderLifecycleState::kSendUnknown || to == OrderLifecycleState::kCancelPending;
    case OrderLifecycleState::kEmsQueued:
      return to == OrderLifecycleState::kSendPending || to == OrderLifecycleState::kRejected ||
             to == OrderLifecycleState::kCancelPending;
    case OrderLifecycleState::kSendPending:
      return to == OrderLifecycleState::kWorking || to == OrderLifecycleState::kRejected ||
             to == OrderLifecycleState::kSendUnknown || to == OrderLifecycleState::kPartiallyFilled ||
             to == OrderLifecycleState::kFilled;
    case OrderLifecycleState::kSendUnknown:
      return to == OrderLifecycleState::kWorking || to == OrderLifecycleState::kRejected ||
             to == OrderLifecycleState::kPartiallyFilled || to == OrderLifecycleState::kFilled ||
             to == OrderLifecycleState::kCanceled;
    case OrderLifecycleState::kWorking:
      return to == OrderLifecycleState::kPartiallyFilled || to == OrderLifecycleState::kFilled ||
             to == OrderLifecycleState::kCancelPending || to == OrderLifecycleState::kCanceled ||
             to == OrderLifecycleState::kRejected;
    case OrderLifecycleState::kPartiallyFilled:
      return to == OrderLifecycleState::kFilled || to == OrderLifecycleState::kCancelPending ||
             to == OrderLifecycleState::kCanceled;
    case OrderLifecycleState::kCancelPending:
      return to == OrderLifecycleState::kCanceled || to == OrderLifecycleState::kWorking ||
             to == OrderLifecycleState::kPartiallyFilled || to == OrderLifecycleState::kFilled ||
             to == OrderLifecycleState::kRejected;
    case OrderLifecycleState::kFilled:
    case OrderLifecycleState::kCanceled:
    case OrderLifecycleState::kRejected:
      return false;
  }
  return false;
}

// =============================================================================
// 内部：柜台快照合并与状态投影
// =============================================================================

void OrderManager::MergeOrderSnapshot(OrderEntry& entry, const trader::Order& report) {
  auto& local = entry.order;
  local.broker_order_id = report.broker_order_id != 0 ? report.broker_order_id : local.broker_order_id;
  local.exchange_order_id = report.exchange_order_id.empty() ? local.exchange_order_id : report.exchange_order_id;
  local.instrument = report.instrument.empty() ? local.instrument : report.instrument;
  if (report.market != trader::MarketType::kUnknown && report.market != trader::MarketType::kInit) {
    local.market = report.market;
  }
  if (report.price > 0.0) {
    local.price = report.price;
  }
  if (report.volume > 0) {
    local.volume = report.volume;
  }
  local.status = report.status;
  local.submit_status = report.submit_status;
  local.traded_volume = report.traded_volume;
  local.left_volume = report.left_volume;
  local.trade_amount = report.trade_amount;
  local.error_message = report.error_message;
  local.side = report.side;
  local.price_type = report.price_type;
  local.position_effect = report.position_effect;
  local.business_type = report.business_type;
}

OrderLifecycleState OrderManager::LifecycleFromOrderStatus(trader::OrderStatusType status,
                                                           OrderLifecycleState fallback) {
  if (status == trader::OrderStatusType::kFilled) {
    return OrderLifecycleState::kFilled;
  }
  if (status == trader::OrderStatusType::kPartiallyFilled ||
      status == trader::OrderStatusType::kPartiallyFilledNotQueueing) {
    return OrderLifecycleState::kPartiallyFilled;
  }
  if (status == trader::OrderStatusType::kCanceled) {
    return OrderLifecycleState::kCanceled;
  }
  if (status == trader::OrderStatusType::kRejected) {
    return OrderLifecycleState::kRejected;
  }
  if (status == trader::OrderStatusType::kUnknown) {
    return OrderLifecycleState::kSendUnknown;
  }
  return fallback == OrderLifecycleState::kPrepared ? OrderLifecycleState::kWorking : fallback;
}

}  // namespace qtrade::engine::orders
