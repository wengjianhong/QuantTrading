/// @file      sdk_event_handler.cpp
/// @brief     SDK 回调入站实现
/// @author    wengjianhong
/// @date      2026-08-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/sdk_event_handler.hpp"

#include "qtrade/common/utils/adapter_payload_validation.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine {

using qtrade::common::utils::IsValidBar;
using qtrade::common::utils::IsValidOrder;
using qtrade::common::utils::IsValidTick;
using qtrade::common::utils::IsValidTrade;

SdkEventHandler::SdkEventHandler(std::atomic<bool>& running,
                                 events::EventLanes& event_lanes,
                                 QuoteHealthMonitor& quote_health_monitor)
  : running_(running), event_lanes_(event_lanes), quote_health_monitor_(quote_health_monitor) {}

void SdkEventHandler::OnTick(const qtrade::sdk::quote::MarketTick& tick) {
  if (!running_.load(std::memory_order_acquire)) {
    return;
  }
  if (!IsValidTick(tick)) {
    quote_health_monitor_.OnInvalidTick();
    spdlog::debug("rejected invalid tick: instrument={}", tick.instrument);
    return;
  }
  quote_health_monitor_.OnValidTick();
  event_lanes_.Quote().PublishTick(tick);
}

void SdkEventHandler::OnBar(const qtrade::sdk::quote::Bar& bar) {
  if (!running_.load(std::memory_order_acquire) || !IsValidBar(bar)) {
    return;
  }
  event_lanes_.Quote().PublishBar(bar);
}

void SdkEventHandler::OnOrder(const qtrade::sdk::trader::Order& order) {
  if (!running_.load(std::memory_order_acquire) || !IsValidOrder(order)) {
    return;
  }
  event_lanes_.Trader().PublishOrder(order);
}

void SdkEventHandler::OnTrade(const qtrade::sdk::trader::Trade& trade) {
  if (!running_.load(std::memory_order_acquire) || !IsValidTrade(trade)) {
    return;
  }
  event_lanes_.Trader().PublishTrade(trade);
}

}  // namespace qtrade::engine
