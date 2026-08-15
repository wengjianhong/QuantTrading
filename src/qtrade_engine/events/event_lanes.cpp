/// @file      event_lanes.cpp
/// @brief     EventLanes 启停与访问器实现
/// @details   依次启动/停止 Market 与 Return 两条 EventReactor；Stop 时先停 Return 再停 Market
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include "qtrade/engine/events/event_lanes.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::events {

void EventLanes::Start() {
  // 1. 先建立行情通道，再建立交易回报通道
  quote_event_reactor_.Start();
  trader_event_reactor_.Start();
  spdlog::info("[EventLanes] Quote + Trader event reactors started");
}

void EventLanes::Stop() {
  // 1. 优先停止交易回报通道，避免关闭期间继续修改交易状态
  trader_event_reactor_.Stop();
  // 2. 再停止行情通道并释放订阅回调
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

}  // namespace qtrade::engine::events
