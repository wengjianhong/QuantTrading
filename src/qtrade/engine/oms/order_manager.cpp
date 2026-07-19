/// @file      order_manager.cpp
/// @brief     订单管理器实现
/// @details   实现订单创建、修改、撤销及状态跟踪
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/oms/order_manager.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>

namespace qtrade::engine::oms {

namespace trader = qtrade_sdk::trader;
namespace {

/// @brief 生成成交幂等键
/// @param trade 成交回报
/// @return trade_id 优先；否则由订单与成交字段拼接
std::string TradeDedupKey(const trader::Trade& trade) {
  return !trade.trade_id.empty()
           ? trade.trade_id
           : trade.order_id + ":" + std::to_string(trade.report_index) + ":" +
               std::to_string(trade.client_order_id) + ":" + trade.instrument + ":" +
               std::to_string(trade.trade_time) + ":" + std::to_string(trade.price) + ":" +
               std::to_string(trade.volume);
}

}  // namespace

OrderManager::OrderManager() : order_id_counter_(0) {}

OrderManager::~OrderManager() {
  Shutdown();
}

ErrorCode OrderManager::Initialize(const OrderManagerOptions& options) {
  // 1. 校验选项并打开主日志
  if (options.tenant_id.empty() || options.engine_id.empty() || options.engine_epoch == 0 ||
      options.journal_path.empty()) {
    return ErrorCode::kSystemError;
  }
  {
    std::lock_guard lock(mutex_);
    if (journal_.IsOpen()) {
      return ErrorCode::kSystemError;
    }
  }
  if (const auto result = journal_.Open(options.journal_path, options.fsync_on_append);
      result != ErrorCode::kSuccess) {
    return result;
  }

  // 2. 回放日志重建订单表、索引与成交幂等集合
  const auto records = journal_.Replay();
  std::lock_guard lock(mutex_);
  tenant_id_ = options.tenant_id;
  engine_id_ = options.engine_id;
  engine_epoch_ = options.engine_epoch;
  orders_.clear();
  client_order_index_.clear();
  applied_trade_ids_.clear();
  reconciled_order_ids_.clear();
  for (const auto& record : records) {
    if (record.order.order_id.empty()) {
      continue;
    }
    orders_[record.order.order_id] = OrderEntry{record.order, record.lifecycle_state};
    if (record.order.client_order_id != 0) {
      client_order_index_[record.order.client_order_id] = record.order.order_id;
    }
    if (record.trade.has_value()) {
      applied_trade_ids_.insert(TradeDedupKey(*record.trade));
    }
  }
  order_id_counter_.store(records.size(), std::memory_order_release);
  spdlog::info("[OrderManager] recovered {} orders from {}", orders_.size(), options.journal_path);
  return ErrorCode::kSuccess;
}

void OrderManager::Shutdown() {
  journal_.Close();
  spdlog::info("[OrderManager] shutdown");
}

ErrorCode OrderManager::SendOrder(const trader::OrderRequest& request) {
  return CreateOrder(request).has_value() ? ErrorCode::kSuccess : ErrorCode::kNotInitialized;
}

std::optional<trader::Order> OrderManager::CreateOrder(const trader::OrderRequest& request) {
  return CreateOrder(request, AllocateOrderId());
}

std::string OrderManager::AllocateOrderId() {
  const auto sequence = order_id_counter_.fetch_add(1, std::memory_order_acq_rel) + 1;
  return tenant_id_ + "-" + engine_id_ + "-" + std::to_string(engine_epoch_) + "-" + std::to_string(sequence);
}

std::optional<trader::Order> OrderManager::CreateOrder(const trader::OrderRequest& request,
                                                       const std::string& order_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!journal_.IsOpen() || order_id.empty()) {
    return std::nullopt;
  }
  // 1. 同 client_order_id 幂等返回已有订单
  if (request.client_order_id != 0) {
    const auto existing = client_order_index_.find(request.client_order_id);
    if (existing != client_order_index_.end()) {
      return orders_.at(existing->second).order;
    }
  }

  // 2. 组装 Prepared 快照并先落盘再入内存
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

  OrderJournalRecord record;
  record.engine_epoch = engine_epoch_;
  record.event_type = OrderJournalEventType::kPrepared;
  record.order = entry.order;
  record.lifecycle_state = entry.lifecycle_state;
  if (journal_.Append(std::move(record)) != ErrorCode::kSuccess) {
    spdlog::error("[OrderManager] persist prepared order failed: {}", order_id);
    return std::nullopt;
  }

  orders_[order_id] = entry;
  if (request.client_order_id != 0) {
    client_order_index_[request.client_order_id] = order_id;
  }
  spdlog::info("[OrderManager] order created: {}", order_id);
  return entry.order;
}

ErrorCode OrderManager::CancelOrder(const std::string& order_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  return PersistTransition(
    it->second, OrderLifecycleState::kCancelPending, OrderJournalEventType::kCancelRequested);
}

ErrorCode OrderManager::MarkEmsQueued(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  return PersistTransition(it->second, OrderLifecycleState::kEmsQueued, OrderJournalEventType::kEmsQueued);
}

ErrorCode OrderManager::MarkSendPending(const std::string& order_id) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  it->second.order.submit_status = trader::OrderSubmitStatusType::kInsertSubmitted;
  return PersistTransition(
    it->second, OrderLifecycleState::kSendPending, OrderJournalEventType::kSendPending);
}

ErrorCode OrderManager::RecordSendResult(const std::string& order_id, ErrorCode result) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }

  // 按通道返回码映射目标生命周期：超时进 SendUnknown，失败进 Rejected
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
  return PersistTransition(it->second,
                           target_state,
                           OrderJournalEventType::kSendResult,
                           std::to_string(static_cast<int>(result)));
}

ErrorCode OrderManager::RecordCancelResult(const std::string& order_id, ErrorCode result) {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return ErrorCode::kNotFound;
  }
  if (result == ErrorCode::kSuccess || result == ErrorCode::kTimeout) {
    return PersistTransition(it->second,
                             OrderLifecycleState::kCancelPending,
                             OrderJournalEventType::kCancelResult,
                             std::to_string(static_cast<int>(result)));
  }

  it->second.order.submit_status = trader::OrderSubmitStatusType::kCancelRejected;
  const auto fallback = it->second.order.traded_volume > 0 ? OrderLifecycleState::kPartiallyFilled
                                                           : OrderLifecycleState::kWorking;
  return PersistTransition(it->second,
                           fallback,
                           OrderJournalEventType::kCancelResult,
                           std::to_string(static_cast<int>(result)));
}

std::optional<trader::Order> OrderManager::GetOrder(const std::string& order_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = orders_.find(order_id);
  if (it != orders_.end()) {
    return it->second.order;
  }
  return std::nullopt;
}

std::optional<trader::Order> OrderManager::GetOrderByClientId(std::uint32_t client_order_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto index_it = client_order_index_.find(client_order_id);
  if (index_it == client_order_index_.end()) {
    return std::nullopt;
  }
  const auto order_it = orders_.find(index_it->second);
  return order_it == orders_.end() ? std::nullopt : std::optional<trader::Order>(order_it->second.order);
}

std::optional<OrderLifecycleState> OrderManager::GetLifecycleState(const std::string& order_id) const {
  std::lock_guard lock(mutex_);
  const auto it = orders_.find(order_id);
  return it == orders_.end() ? std::nullopt : std::optional<OrderLifecycleState>(it->second.lifecycle_state);
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
    if (entry.lifecycle_state != OrderLifecycleState::kFilled &&
        entry.lifecycle_state != OrderLifecycleState::kCanceled &&
        entry.lifecycle_state != OrderLifecycleState::kRejected) {
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
    if (entry.lifecycle_state != OrderLifecycleState::kFilled &&
        entry.lifecycle_state != OrderLifecycleState::kCanceled &&
        entry.lifecycle_state != OrderLifecycleState::kRejected) {
      notional += entry.order.price * static_cast<double>(entry.order.left_volume);
    }
  }
  return notional;
}

void OrderManager::UpdateOrderStatus(const std::string& order_id, trader::OrderStatusType status) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = orders_.find(order_id);
  if (it != orders_.end()) {
    it->second.order.status = status;
    OrderLifecycleState target = it->second.lifecycle_state;
    if (status == trader::OrderStatusType::kFilled) {
      target = OrderLifecycleState::kFilled;
    } else if (status == trader::OrderStatusType::kCanceled) {
      target = OrderLifecycleState::kCanceled;
    } else if (status == trader::OrderStatusType::kRejected) {
      target = OrderLifecycleState::kRejected;
    }
    (void)PersistTransition(it->second, target, OrderJournalEventType::kOrderReport);
    spdlog::info("[OrderManager] order {} status updated to {}", order_id, static_cast<int>(status));
  }
}

void OrderManager::ApplyOrderReport(const trader::Order& report) {
  std::lock_guard<std::mutex> lock(mutex_);
  // 1. 按 order_id / client_order_id 定位本地订单
  auto it = orders_.find(report.order_id);
  if (it == orders_.end() && report.client_order_id != 0) {
    const auto index_it = client_order_index_.find(report.client_order_id);
    if (index_it != client_order_index_.end()) {
      it = orders_.find(index_it->second);
    }
  }
  if (it == orders_.end()) {
    spdlog::warn("[OrderManager] ignored order report without local order, client_order_id={}", report.client_order_id);
    return;
  }

  // 2. 合并柜台字段并映射生命周期后持久化
  OrderEntry updated = it->second;
  auto& local = updated.order;
  local.order_emt_id = report.order_emt_id != 0 ? report.order_emt_id : local.order_emt_id;
  local.exchange_order_id = report.exchange_order_id.empty() ? local.exchange_order_id : report.exchange_order_id;
  local.status = report.status;
  local.submit_status = report.submit_status;
  local.traded_volume = report.traded_volume;
  local.left_volume = report.left_volume;
  local.trade_amount = report.trade_amount;
  local.error_message = report.error_message;

  OrderLifecycleState target = OrderLifecycleState::kWorking;
  if (report.status == trader::OrderStatusType::kFilled) {
    target = OrderLifecycleState::kFilled;
  } else if (report.status == trader::OrderStatusType::kPartiallyFilled ||
             report.status == trader::OrderStatusType::kPartiallyFilledNotQueueing) {
    target = OrderLifecycleState::kPartiallyFilled;
  } else if (report.status == trader::OrderStatusType::kCanceled) {
    target = OrderLifecycleState::kCanceled;
  } else if (report.status == trader::OrderStatusType::kRejected) {
    target = OrderLifecycleState::kRejected;
  }
  if (PersistTransition(updated, target, OrderJournalEventType::kOrderReport) == ErrorCode::kSuccess) {
    it->second = std::move(updated);
  }
}

void OrderManager::ApplyTradeReport(const trader::Trade& report) {
  std::lock_guard<std::mutex> lock(mutex_);
  // 1. 成交幂等与订单定位
  const std::string dedup_key = TradeDedupKey(report);
  if (applied_trade_ids_.contains(dedup_key)) {
    return;
  }
  auto it = orders_.find(report.order_id);
  if (it == orders_.end() && report.client_order_id != 0) {
    const auto index_it = client_order_index_.find(report.client_order_id);
    if (index_it != client_order_index_.end()) {
      it = orders_.find(index_it->second);
    }
  }
  if (it == orders_.end()) {
    spdlog::warn("[OrderManager] ignored trade report without local order, client_order_id={}", report.client_order_id);
    return;
  }

  // 2. 累加成交量并持久化部分/全部成交状态
  OrderEntry updated = it->second;
  trader::Order& local = updated.order;
  local.traded_volume += report.volume;
  local.left_volume = std::max<std::int64_t>(0, local.volume - local.traded_volume);
  local.trade_amount += report.trade_amount;
  local.status = local.left_volume == 0 ? trader::OrderStatusType::kFilled : trader::OrderStatusType::kPartiallyFilled;
  const auto target =
    local.left_volume == 0 ? OrderLifecycleState::kFilled : OrderLifecycleState::kPartiallyFilled;
  if (PersistTransition(updated, target, OrderJournalEventType::kTradeReport, {}, report) ==
      ErrorCode::kSuccess) {
    it->second = std::move(updated);
    applied_trade_ids_.insert(dedup_key);
  }
}

ErrorCode OrderManager::PersistTransition(OrderEntry& entry,
                                          OrderLifecycleState target_state,
                                          OrderJournalEventType event_type,
                                          const std::string& message,
                                          const std::optional<trader::Trade>& trade) {
  // 1. 校验状态机迁移合法性
  if (!CanTransition(entry.lifecycle_state, target_state)) {
    spdlog::error("[OrderManager] invalid state transition: order={}, from={}, to={}",
                  entry.order.order_id,
                  static_cast<int>(entry.lifecycle_state),
                  static_cast<int>(target_state));
    return ErrorCode::kSystemError;
  }

  // 2. 先写主日志成功后再更新内存状态
  OrderJournalRecord record;
  record.engine_epoch = engine_epoch_;
  record.event_type = event_type;
  record.order = entry.order;
  record.lifecycle_state = target_state;
  record.trade = trade;
  record.message = message;
  if (const auto result = journal_.Append(std::move(record)); result != ErrorCode::kSuccess) {
    return result;
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
             to == OrderLifecycleState::kCancelPending;
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

}  // namespace qtrade::engine::oms
