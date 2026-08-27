/// @file      order_intent_queue.hpp
/// @brief     OrderIntent 有界队列：A 段入队，独立线程执行 E 段预占、OMS 与 EMS 入队
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ORDER_INTENT_QUEUE_HPP_
#define QTRADE_ENGINE_ORDER_INTENT_QUEUE_HPP_

#include "qtrade/engine/account_risk/account_risk_api.hpp"
#include "qtrade/engine/execution/execution_api.hpp"
#include "qtrade/engine/orders/order_api.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace qtrade::engine {

/// @brief A 段已放行、尚未账户预占的发单意图
struct OrderIntent {
  /// 下单请求
  qtrade::sdk::trader::OrderRequest request;
};

/// @brief E 段终态/失败回传（直接入策略队列，不经 Lane-T）
using OrderOutcomeHandler = std::function<void(const qtrade::sdk::trader::Order&)>;

/// @brief OrderIntent 有界队列 + E 段工作线程
class OrderIntentQueue {
 public:
  /// 默认队列容量
  static constexpr std::size_t kDefaultCapacity = 8192;

  /// @brief 绑定 E 段依赖（不拥有）
  /// @param account_risk 账户硬风控
  /// @param orders OMS
  /// @param execution EMS
  /// @param capacity 队列容量；0 按默认容量
  OrderIntentQueue(account_risk::AccountRiskApi& account_risk,
                   orders::OrderApi& orders,
                   execution::ExecutionApi& execution,
                   std::size_t capacity = kDefaultCapacity);

  /// @brief 停止工作线程
  ~OrderIntentQueue();

  /// @brief 禁止移动构造
  OrderIntentQueue(OrderIntentQueue&&) = delete;
  /// @brief 禁止拷贝构造
  OrderIntentQueue(const OrderIntentQueue&) = delete;
  /// @brief 禁止移动赋值
  OrderIntentQueue& operator=(OrderIntentQueue&&) = delete;
  /// @brief 禁止拷贝赋值
  OrderIntentQueue& operator=(const OrderIntentQueue&) = delete;

  /// @brief 设置 E 段失败/未知回传；须在 Start 前调用
  /// @param handler 将 Order 送入对应策略队列；空表示只记 OMS、不通知策略
  void SetOutcomeHandler(OrderOutcomeHandler handler);

  /// @brief 启动 E 段工作线程
  void Start();

  /// @brief 停止接收入队、排空后 join 工作线程
  void Stop();

  /// @brief 将已通过 A 段的意图入队
  /// @param intent 发单意图
  /// @return 入队成功或同 client_order_id 已在途/已建单返回 kSuccess；未启动返回 kNotInitialized；队列满返回
  /// kResourceExhausted
  ErrorCode Enqueue(OrderIntent intent);

  /// @brief 队列中尚未完成 E 段的意图数（含正在 Execute 的一笔）
  [[nodiscard]] std::uint64_t PendingCount() const;

  /// @brief 在途意图累计名义（price * volume）
  [[nodiscard]] double PendingNotional() const;

 private:
  /// @brief 工作线程主循环
  void Run();

  /// @brief 执行 E 段：预占 → OMS → EMS 入队
  /// @param intent 发单意图
  void Execute(const OrderIntent& intent);

  /// @brief 入队对应的在途计数结束（含失败路径）
  /// @param client_order_id 客户端订单 ID；0 表示不参与幂等窗口
  /// @param notional 本笔名义
  void CompleteIntent(std::uint32_t client_order_id, double notional);

  /// @brief 将失败/未知写入 OMS 并回传策略
  /// @param request 原始请求
  /// @param order_id 本次分配的订单 ID
  /// @param rc 失败原因；kTimeout 记 Unknown，其余记 Rejected
  void RecordLocalFailure(const qtrade::sdk::trader::OrderRequest& request, const std::string& order_id, ErrorCode rc);

  /// @brief 回传策略（不经 Lane-T）
  /// @param order 订单快照
  void NotifyOutcome(const qtrade::sdk::trader::Order& order) const;

  /// 队列容量
  std::size_t capacity_ = kDefaultCapacity;
  /// 账户硬风控
  account_risk::AccountRiskApi& account_risk_;
  /// OMS
  orders::OrderApi& orders_;
  /// EMS
  execution::ExecutionApi& execution_;
  /// E 段失败/未知回传
  OrderOutcomeHandler outcome_handler_;
  /// 待执行意图
  std::deque<OrderIntent> queue_;
  /// 已入队尚未 CreateOrder 的 client_order_id
  std::unordered_set<std::uint32_t> inflight_client_ids_;
  /// 保护队列、在途集合与运行状态
  std::mutex mutex_;
  /// 队列非空或停止时唤醒
  std::condition_variable cv_;
  /// E 段工作线程
  std::thread worker_;
  /// 是否已 Start
  std::atomic<bool> running_{false};
  /// 是否仍接受入队
  std::atomic<bool> accepting_{false};
  /// 在途意图数
  std::atomic<std::uint64_t> pending_count_{0};
  /// 在途名义
  std::atomic<double> pending_notional_{0.0};
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ORDER_INTENT_QUEUE_HPP_
