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
#include <deque>
#include <mutex>
#include <thread>

namespace qtrade::engine {

/// @brief A 段已放行、尚未账户预占的发单意图
struct OrderIntent {
  /// 下单请求
  qtrade::sdk::trader::OrderRequest request;
};

/// @brief OrderIntent 有界队列 + E 段工作线程
class OrderIntentQueue {
 public:
  /// @brief 绑定 E 段依赖（不拥有）
  /// @param account_risk 账户硬风控
  /// @param orders OMS
  /// @param execution EMS
  OrderIntentQueue(account_risk::AccountRiskApi& account_risk,
                   orders::OrderApi& orders,
                   execution::ExecutionApi& execution);

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

  /// @brief 启动 E 段工作线程
  void Start();

  /// @brief 停止接收入队、排空后 join 工作线程
  void Stop();

  /// @brief 将已通过 A 段的意图入队
  /// @param intent 发单意图
  /// @return 入队成功返回 kSuccess；未启动返回 kNotInitialized；队列满返回 kResourceExhausted
  ErrorCode Enqueue(OrderIntent intent);

 private:
  /// @brief 工作线程主循环
  void Run();

  /// @brief 执行 E 段：预占 → OMS → EMS 入队
  /// @param intent 发单意图
  void Execute(const OrderIntent& intent);

  /// 队列容量
  static constexpr std::size_t kQueueCapacity = 8192;
  /// 账户硬风控
  account_risk::AccountRiskApi& account_risk_;
  /// OMS
  orders::OrderApi& orders_;
  /// EMS
  execution::ExecutionApi& execution_;
  /// 待执行意图
  std::deque<OrderIntent> queue_;
  /// 保护队列与运行状态
  std::mutex mutex_;
  /// 队列非空或停止时唤醒
  std::condition_variable cv_;
  /// E 段工作线程
  std::thread worker_;
  /// 是否已 Start
  std::atomic<bool> running_{false};
  /// 是否仍接受入队
  std::atomic<bool> accepting_{false};
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ORDER_INTENT_QUEUE_HPP_
