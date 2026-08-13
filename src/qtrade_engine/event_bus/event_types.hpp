/// @file      event_types.hpp
/// @brief     EventBus 事件类型与载荷结构
/// @details   定义 EventType、Event 基类/派生事件
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EVENT_TYPES_HPP_
#define QTRADE_TRADING_ENGINE_EVENT_TYPES_HPP_

#include <qtrade/sdk/quote/quote_struct.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <memory>

namespace qtrade::engine::event_bus {

/// @brief 事件类型枚举
enum class EventType {
  /// 行情 Tick 事件（Lane-Q）
  kTickData = 0,
  /// K 线 Bar 事件（Lane-Q）
  kBarData = 1,
  /// 订单回报事件（Lane-T）
  kOrderUpdate = 2,
  /// 成交回报事件（Lane-T）
  kTradeUpdate = 3,
};

/// @brief 事件基类；Reactor 队列以 `EventPtr` 多态入队；时间见各子类载荷字段
struct Event {
  /// 事件类型标签
  EventType type;

  /// @brief 构造事件基类
  /// @param t 事件类型
  explicit Event(EventType t);

  /// @brief 虚析构，保证多态删除安全
  virtual ~Event();
};

/// @brief 堆上事件的所有权指针，供 Reactor 队列多态传递
using EventPtr = std::unique_ptr<Event>;

/// @brief Tick 数据事件
struct TickEvent : public Event {
  /// Tick 载荷
  qtrade::sdk::quote::MarketTick tick;

  /// @brief 构造 Tick 事件
  /// @param t 行情 Tick 快照
  explicit TickEvent(const qtrade::sdk::quote::MarketTick& t);
};

/// @brief Bar 数据事件
struct BarEvent : public Event {
  /// Bar 载荷
  qtrade::sdk::quote::Bar bar;

  /// @brief 构造 Bar 事件
  /// @param b K 线 Bar 快照
  explicit BarEvent(const qtrade::sdk::quote::Bar& b);
};

/// @brief 订单更新事件
struct OrderEvent : public Event {
  /// 订单回报载荷
  qtrade::sdk::trader::Order order;

  /// @brief 构造订单更新事件
  /// @param o 订单快照
  explicit OrderEvent(const qtrade::sdk::trader::Order& o);
};

/// @brief 成交更新事件
struct TradeEvent : public Event {
  /// 成交回报载荷
  qtrade::sdk::trader::Trade trade;

  /// @brief 构造成交更新事件
  /// @param t 成交快照
  explicit TradeEvent(const qtrade::sdk::trader::Trade& t);
};

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_EVENT_TYPES_HPP_
