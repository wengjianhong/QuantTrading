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
  market_event_reactor_.Start();
  return_event_reactor_.Start();
  spdlog::info("[EventLanes] Market + Return event reactors started");
}

void EventLanes::Stop() {
  return_event_reactor_.Stop();
  market_event_reactor_.Stop();
  spdlog::info("[EventLanes] stopped cleanly");
}

MarketEventReactor& EventLanes::Market() {
  return market_event_reactor_;
}

ReturnEventReactor& EventLanes::Return() {
  return return_event_reactor_;
}

const MarketEventReactor& EventLanes::Market() const {
  return market_event_reactor_;
}

const ReturnEventReactor& EventLanes::Return() const {
  return return_event_reactor_;
}

std::size_t EventLanes::MarketQueueSize() const {
  return market_event_reactor_.PendingCount();
}

std::size_t EventLanes::ReturnQueueSize() const {
  return return_event_reactor_.PendingCount();
}

}  // namespace qtrade::engine::event_bus
