/// @file      quote_event_reactor.hpp
/// @brief     Lane-Q 行情 EventReactor（EventBus 子系统实现）
/// @details   以 EventPtr FIFO 入队；Reactor 线程按 EventType 分发给 Tick/Bar 订阅者
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_MARKET_EVENT_REACTOR_HPP_
#define QTRADE_TRADING_ENGINE_MARKET_EVENT_REACTOR_HPP_

#include "qtrade/engine/event_bus/event_reactor_loop.hpp"
#include "qtrade/engine/event_bus/event_types.hpp"

#include <cstddef>
#include <mutex>
#include <vector>

namespace qtrade::engine::event_bus {

/// @brief Lane-Q EventReactor：`EventPtr` FIFO 入队，按 `EventType` 调用 EventHandler
class QuoteEventReactor {
 public:
  /// @brief 构造行情 EventReactor
  QuoteEventReactor();

  /// @brief 析构并确保 Reactor 线程已停止
  ~QuoteEventReactor();

  /// @brief 禁止拷贝构造/赋值
  QuoteEventReactor(QuoteEventReactor&&) = delete;
  QuoteEventReactor(const QuoteEventReactor&) = delete;
  QuoteEventReactor& operator=(QuoteEventReactor&&) = delete;
  QuoteEventReactor& operator=(const QuoteEventReactor&) = delete;

  /// @brief 设置队列策略
  /// @note 默认队列容量为 8192，满队列时丢弃最旧事件
  /// @warning 仅未 Start 时生效，Reactor 正在运行时忽略设置
  ///
  /// @param policy 队列容量与满队列处理策略
  /// @return 设置成功返回 true；Reactor 正在运行时返回 false
  bool SetLanePolicy(LanePolicy policy);

  /// @brief 启动 Reactor 线程并开始消费队列
  void Start();

  /// @brief 停止 Reactor 线程并清空已注册的 EventHandler
  void Stop();

  /// @brief 订阅 Tick 事件
  /// @param handler Tick 回调；在 Reactor 线程中调用
  void SubscribeTick(TickEventHandler handler);

  /// @brief 订阅 Bar 事件
  /// @param handler Bar 回调；在 Reactor 线程中调用
  void SubscribeBar(BarEventHandler handler);

  /// @brief 将 Tick 封装为 TickEvent 并入队
  /// @param tick 行情 Tick 快照
  void PublishTick(const qtrade_sdk::quote::MarketTick& tick);

  /// @brief 将 Bar 封装为 BarEvent 并入队
  /// @param bar K 线 Bar 快照
  void PublishBar(const qtrade_sdk::quote::Bar& bar);

  /// @brief 查询是否仍有待处理事件
  /// @return 队列非空时返回 true
  [[nodiscard]] bool HasPending() const;

  /// @brief 获取当前队列深度
  /// @return 待处理事件个数
  [[nodiscard]] std::size_t PendingCount() const;

 private:
  /// @brief 按 EventType 分发给已订阅的 EventHandler
  /// @param event 出队后的事件基类引用
  void HandleEvent(const Event& event);

  /// Lane-Q FIFO Reactor 循环
  EventReactorLoop<EventPtr> loop_;
  /// Tick 事件订阅列表
  std::vector<TickEventHandler> tick_handlers_;
  /// Bar 事件订阅列表
  std::vector<BarEventHandler> bar_handlers_;
  /// 保护订阅列表的互斥锁
  mutable std::mutex handlers_mutex_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_MARKET_EVENT_REACTOR_HPP_
