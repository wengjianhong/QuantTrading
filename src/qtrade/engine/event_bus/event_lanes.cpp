/// @file      event_lanes.cpp
/// @brief     EventLanes 启停实现
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

}  // namespace qtrade::engine::event_bus
