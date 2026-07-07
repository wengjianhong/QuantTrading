/// @file      event_types.hpp
/// @brief     事件类型定义
/// @details   定义事件类型枚举、事件基类以及各类具体事件结构
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EVENT_TYPES_HPP_
#define QTRADE_TRADING_ENGINE_EVENT_TYPES_HPP_

#include <qtrade_sdk/quote/quote_struct.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <functional>
#include <memory>

namespace qtrade::engine::event_bus {

/// @brief 事件类型枚举
enum class EventType {
  kTickData = 0,     ///< 市场 Tick 事件（Lane-M）
  kBarData = 1,      ///< K 线 Bar 事件（Lane-M）
  kOrderUpdate = 2,  ///< 订单回报事件（Lane-R）
  kTradeUpdate = 3,  ///< 成交回报事件（Lane-R）
};

/// @brief 事件基类；Reactor 队列以 `EventPtr` 多态入队；时间见各子类载荷字段
struct Event {
  EventType type;

  explicit Event(EventType t) : type(t) {}
  virtual ~Event() = default;
};

using EventPtr = std::unique_ptr<Event>;

/// @brief Tick 数据事件
struct TickEvent : public Event {
  qtrade_sdk::quote::MarketTick tick;

  explicit TickEvent(const qtrade_sdk::quote::MarketTick& t) : Event(EventType::kTickData), tick(t) {}
};

/// @brief Bar 数据事件
struct BarEvent : public Event {
  qtrade_sdk::quote::Bar bar;

  explicit BarEvent(const qtrade_sdk::quote::Bar& b) : Event(EventType::kBarData), bar(b) {}
};

/// @brief 订单更新事件
struct OrderEvent : public Event {
  qtrade_sdk::trader::Order order;

  explicit OrderEvent(const qtrade_sdk::trader::Order& o) : Event(EventType::kOrderUpdate), order(o) {}
};

/// @brief 成交更新事件
struct TradeEvent : public Event {
  qtrade_sdk::trader::Trade trade;

  explicit TradeEvent(const qtrade_sdk::trader::Trade& t) : Event(EventType::kTradeUpdate), trade(t) {}
};

/// @brief Tick 事件处理器（Lane-M）
using TickEventHandler = std::function<void(const qtrade_sdk::quote::MarketTick&)>;
/// @brief Bar 事件处理器（Lane-M）
using BarEventHandler = std::function<void(const qtrade_sdk::quote::Bar&)>;
/// @brief 订单回报事件处理器（Lane-R）
using OrderEventHandler = std::function<void(const qtrade_sdk::trader::Order&)>;
/// @brief 成交回报事件处理器（Lane-R）
using TradeEventHandler = std::function<void(const qtrade_sdk::trader::Trade&)>;

}  // namespace qtrade::engine::event_bus

#endif  // QTRADE_TRADING_ENGINE_EVENT_TYPES_HPP_
