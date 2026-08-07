/// @file      event_types.cpp
/// @brief     EventBus 事件类型构造与析构实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/event_bus/event_types.hpp"

namespace qtrade::engine::event_bus {

Event::Event(EventType t) : type(t) {}

Event::~Event() = default;

// 1. 行情载荷事件：保留 SDK 快照，供 QuoteEventReactor 异步分发
TickEvent::TickEvent(const qtrade_sdk::quote::MarketTick& t) : Event(EventType::kTickData), tick(t) {}

BarEvent::BarEvent(const qtrade_sdk::quote::Bar& b) : Event(EventType::kBarData), bar(b) {}

// 2. 交易载荷事件：保留订单与成交快照，供 TraderEventReactor 异步分发
OrderEvent::OrderEvent(const qtrade_sdk::trader::Order& o) : Event(EventType::kOrderUpdate), order(o) {}

TradeEvent::TradeEvent(const qtrade_sdk::trader::Trade& t) : Event(EventType::kTradeUpdate), trade(t) {}

}  // namespace qtrade::engine::event_bus
