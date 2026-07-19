/// @file      event_types.cpp
/// @brief     EventBus 事件类型构造与析构实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/event_bus/event_types.hpp"

namespace qtrade::engine::event_bus {

Event::Event(EventType t) : type(t) {}

Event::~Event() = default;

TickEvent::TickEvent(const qtrade_sdk::quote::MarketTick& t) : Event(EventType::kTickData), tick(t) {}

BarEvent::BarEvent(const qtrade_sdk::quote::Bar& b) : Event(EventType::kBarData), bar(b) {}

OrderEvent::OrderEvent(const qtrade_sdk::trader::Order& o) : Event(EventType::kOrderUpdate), order(o) {}

TradeEvent::TradeEvent(const qtrade_sdk::trader::Trade& t) : Event(EventType::kTradeUpdate), trade(t) {}

}  // namespace qtrade::engine::event_bus
