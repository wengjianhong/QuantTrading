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

#include <qtrade/sdk/quote/quote_api.hpp>

#include <cstddef>
#include <mutex>
#include <vector>

namespace qtrade::engine::event_bus {

/// @brief Lane-Q EventReactor：`EventPtr` FIFO 入队，按 `EventType` 调用订阅回调
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

  /// @brief 启动 Reactor 线程并开始消费队列
  void Start();

  /// @brief 停止 Reactor 线程并清空已注册的回调
  void Stop();

  /// @brief 查询是否仍有待处理事件
  /// @return 队列非空时返回 true
  [[nodiscard]] bool HasPending() const;

  /// @brief 获取当前队列深度
  /// @return 待处理事件个数
  [[nodiscard]] std::size_t PendingCount() const;

  /// @brief 注册 Tick 回调
  /// @param callback Tick 回调；在 Reactor 线程中调用
  void RegisterTickCallback(qtrade::sdk::quote::QuoteApi::TickCallback callback);

  /// @brief 注册 Bar 回调
  /// @param callback Bar 回调；在 Reactor 线程中调用
  void RegisterBarCallback(qtrade::sdk::quote::QuoteApi::BarCallback callback);

  /// @brief 将 Tick 封装为 TickEvent 并入队
  /// @param tick 行情 Tick 快照
  void PublishTick(const qtrade::sdk::quote::MarketTick& tick);

  /// @brief 将 Bar 封装为 BarEvent 并入队
  /// @param bar K 线 Bar 快照
  void PublishBar(const qtrade::sdk::quote::Bar& bar);

 private:
  /// @brief 按 EventType 分发给已订阅的回调
  /// @param event 出队后的事件基类引用
  void HandleEvent(const Event& event);

  /// Lane-Q FIFO Reactor 循环
  EventReactorLoop<EventPtr> loop_;
  /// 保护订阅列表的互斥锁
  mutable std::mutex callbacks_mutex_;
  /// Tick 事件订阅列表
  std::vector<qtrade::sdk::quote::QuoteApi::TickCallback> tick_callbacks_;
  /// Bar 事件订阅列表
  std::vector<qtrade::sdk::quote::QuoteApi::BarCallback> bar_callbacks_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_MARKET_EVENT_REACTOR_HPP_
