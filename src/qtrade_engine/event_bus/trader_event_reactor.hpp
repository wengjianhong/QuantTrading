/// @file      trader_event_reactor.hpp
/// @brief     Lane-T 回报 EventReactor（EventBus 子系统实现）
/// @details   以 EventPtr FIFO 入队；Reactor 线程按 EventType 分发给 Order/Trade 订阅者
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_RETURN_EVENT_REACTOR_HPP_
#define QTRADE_TRADING_ENGINE_RETURN_EVENT_REACTOR_HPP_

#include "qtrade/engine/event_bus/event_reactor_loop.hpp"
#include "qtrade/engine/event_bus/event_types.hpp"

#include <qtrade/sdk/trader/trader_api.hpp>

#include <cstddef>
#include <mutex>
#include <vector>

namespace qtrade::engine::event_bus {

/// @brief Lane-T EventReactor：`EventPtr` FIFO 入队，按 `EventType` 调用订阅回调
class TraderEventReactor {
 public:
  /// @brief 构造回报 EventReactor
  TraderEventReactor();

  /// @brief 析构并确保 Reactor 线程已停止
  ~TraderEventReactor();

  /// @brief 禁止拷贝构造/赋值
  TraderEventReactor(TraderEventReactor&&) = delete;
  TraderEventReactor(const TraderEventReactor&) = delete;
  TraderEventReactor& operator=(TraderEventReactor&&) = delete;
  TraderEventReactor& operator=(const TraderEventReactor&) = delete;

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

  /// @brief 注册订单回报事件
  /// @param callback 订单回调；在 Reactor 线程中调用
  void RegisterOrderCallback(qtrade::sdk::trader::TraderApi::OrderCallback callback);

  /// @brief 注册成交回报事件
  /// @param callback 成交回调；在 Reactor 线程中调用
  void RegisterTradeCallback(qtrade::sdk::trader::TraderApi::TradeCallback callback);

  /// @brief 将订单封装为 OrderEvent 并入队
  /// @param order 订单回报快照
  void PublishOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 将成交封装为 TradeEvent 并入队
  /// @param trade 成交回报快照
  void PublishTrade(const qtrade::sdk::trader::Trade& trade);

 private:
  /// @brief 按 EventType 分发给已订阅的回调
  /// @param event 出队后的事件基类引用
  void HandleEvent(const Event& event);

  /// Lane-T FIFO Reactor 循环
  EventReactorLoop<EventPtr> loop_;
  /// 保护订阅列表的互斥锁
  mutable std::mutex callbacks_mutex_;
  /// 订单回报订阅列表
  std::vector<qtrade::sdk::trader::TraderApi::OrderCallback> order_callbacks_;
  /// 成交回报订阅列表
  std::vector<qtrade::sdk::trader::TraderApi::TradeCallback> trade_callbacks_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_RETURN_EVENT_REACTOR_HPP_
