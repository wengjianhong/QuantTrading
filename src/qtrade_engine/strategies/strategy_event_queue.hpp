/// @file      strategy_event_queue.hpp
/// @brief     每策略一条串行事件队列：Tick/Bar/Order/Trade 入队后由本策略线程回调
/// @details   Lane-Q / Lane-T 只入队并立即返回；策略 On* 在独立 worker 上串行执行。
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_STRATEGY_EVENT_QUEUE_HPP_
#define QTRADE_ENGINE_STRATEGY_EVENT_QUEUE_HPP_

#include <qtrade/sdk/quote/quote_struct.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <thread>
#include <variant>

namespace qtrade::engine::strategies {

/// @brief 策略入站事件（行情或回报）
using StrategyEvent = std::variant<qtrade::sdk::quote::MarketTick,
                                   qtrade::sdk::quote::Bar,
                                   qtrade::sdk::trader::Order,
                                   qtrade::sdk::trader::Trade>;

/// @brief 单策略有界串行队列 + 工作线程
class StrategyEventQueue {
 public:
  /// @brief 绑定策略实例（不拥有）
  /// @param strategy 策略回调目标；须在 Stop 之后才可销毁
  explicit StrategyEventQueue(qtrade::strategy::IStrategy& strategy);

  /// @brief 停止工作线程
  ~StrategyEventQueue();

  /// @brief 禁止移动构造
  StrategyEventQueue(StrategyEventQueue&&) = delete;
  /// @brief 禁止拷贝构造
  StrategyEventQueue(const StrategyEventQueue&) = delete;
  /// @brief 禁止移动赋值
  StrategyEventQueue& operator=(StrategyEventQueue&&) = delete;
  /// @brief 禁止拷贝赋值
  StrategyEventQueue& operator=(const StrategyEventQueue&) = delete;

  /// @brief 启动策略工作线程
  void Start();

  /// @brief 停止接收入队并 join 工作线程
  void Stop();

  /// @brief 入队 Tick；满队列时丢弃最旧事件
  /// @param tick 行情 tick
  /// @return 已入队返回 true；未启动或停写返回 false
  bool EnqueueTick(const qtrade::sdk::quote::MarketTick& tick);

  /// @brief 入队 Bar；满队列时丢弃最旧事件
  /// @param bar K 线
  /// @return 已入队返回 true；未启动或停写返回 false
  bool EnqueueBar(const qtrade::sdk::quote::Bar& bar);

  /// @brief 入队订单回报；满队列时丢弃最旧事件
  /// @param order 订单回报
  /// @return 已入队返回 true；未启动或停写返回 false
  bool EnqueueOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 入队成交回报；满队列时丢弃最旧事件
  /// @param trade 成交回报
  /// @return 已入队返回 true；未启动或停写返回 false
  bool EnqueueTrade(const qtrade::sdk::trader::Trade& trade);

 private:
  /// @brief 将事件写入有界队列
  /// @param event 待入队事件
  /// @return 已入队返回 true；未启动或停写返回 false
  bool Enqueue(StrategyEvent event);

  /// @brief 工作线程主循环
  void Run();

  /// @brief 按事件类型回调策略
  /// @param event 出队事件
  void Dispatch(const StrategyEvent& event);

  /// 队列容量
  static constexpr std::size_t kQueueCapacity = 8192;
  /// 策略实例（不拥有）
  qtrade::strategy::IStrategy& strategy_;
  /// 待回调事件
  std::deque<StrategyEvent> queue_;
  /// 保护队列与运行状态
  std::mutex mutex_;
  /// 队列非空或停止时唤醒
  std::condition_variable cv_;
  /// 策略工作线程
  std::thread worker_;
  /// 是否已 Start
  std::atomic<bool> running_{false};
  /// 是否仍接受入队
  std::atomic<bool> accepting_{false};
};

}  // namespace qtrade::engine::strategies

#endif  // QTRADE_ENGINE_STRATEGY_EVENT_QUEUE_HPP_
