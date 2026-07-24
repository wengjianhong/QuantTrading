/// @file      event_lanes.cpp
/// @brief     EventLanes 启停与访问器实现
/// @details   依次启动/停止 Market 与 Return 两条 EventReactor；Stop 时先停 Return 再停 Market
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/event_bus/event_lanes.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::event_bus {

void EventLanes::Start() {
  quote_event_reactor_.Start();
  trader_event_reactor_.Start();
  spdlog::info("[EventLanes] Market + Return event reactors started");
}

void EventLanes::Stop() {
  trader_event_reactor_.Stop();
  quote_event_reactor_.Stop();
  spdlog::info("[EventLanes] stopped cleanly");
}

QuoteEventReactor& EventLanes::Quote() {
  return quote_event_reactor_;
}

TraderEventReactor& EventLanes::Trader() {
  return trader_event_reactor_;
}

const QuoteEventReactor& EventLanes::Quote() const {
  return quote_event_reactor_;
}

const TraderEventReactor& EventLanes::Trader() const {
  return trader_event_reactor_;
}

std::size_t EventLanes::QuoteQueueSize() const {
  return quote_event_reactor_.PendingCount();
}

std::size_t EventLanes::TraderQueueSize() const {
  return trader_event_reactor_.PendingCount();
}

}  // namespace qtrade::engine::event_bus
