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

#include <cstddef>
#include <mutex>
#include <vector>

namespace qtrade::engine::event_bus {

/// @brief Lane-T EventReactor：`EventPtr` FIFO 入队，按 `EventType` 调用 EventHandler
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

  /// @brief 订阅订单回报事件
  /// @param handler 订单回调；在 Reactor 线程中调用
  void SubscribeOrder(OrderEventHandler handler);

  /// @brief 订阅成交回报事件
  /// @param handler 成交回调；在 Reactor 线程中调用
  void SubscribeTrade(TradeEventHandler handler);

  /// @brief 将订单封装为 OrderEvent 并入队
  /// @param order 订单回报快照
  void PublishOrder(const qtrade::sdk::trader::Order& order);

  /// @brief 将成交封装为 TradeEvent 并入队
  /// @param trade 成交回报快照
  void PublishTrade(const qtrade::sdk::trader::Trade& trade);

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

  /// Lane-T FIFO Reactor 循环
  EventReactorLoop<EventPtr> loop_;
  /// 订单回报订阅列表
  std::vector<OrderEventHandler> order_handlers_;
  /// 成交回报订阅列表
  std::vector<TradeEventHandler> trade_handlers_;
  /// 保护订阅列表的互斥锁
  mutable std::mutex handlers_mutex_;
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_RETURN_EVENT_REACTOR_HPP_
