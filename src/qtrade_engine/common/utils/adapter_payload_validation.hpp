/// @file      adapter_payload_validation.hpp
/// @brief     行情/交易 SDK 回调入站校验
/// @details   在 WireQuoteCallbacks / WireTraderCallbacks 边界过滤脏数据，
///            避免无效 tick 污染行情健康度、无效回报进入 Lane-Q/Lane-T。
/// @author    wengjianhong
/// @date      2026-08-05
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_UTILS_ADAPTER_PAYLOAD_VALIDATION_HPP_
#define QTRADE_COMMON_UTILS_ADAPTER_PAYLOAD_VALIDATION_HPP_

#include <qtrade/sdk/quote/quote_struct.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <cmath>

namespace qtrade::common::utils {

/// @brief 校验 Tick 是否可进入 Lane-Q
[[nodiscard]] inline bool IsValidTick(const qtrade_sdk::quote::MarketTick& tick) {
  return !tick.instrument.empty() && tick.data_time > 0 && std::isfinite(tick.last_price) && tick.last_price > 0.0 &&
         tick.volume >= 0;
}

/// @brief 校验 Bar 是否可进入 Lane-Q
[[nodiscard]] inline bool IsValidBar(const qtrade_sdk::quote::Bar& bar) {
  return !bar.instrument.empty() && bar.open_time > 0 && bar.close_time >= bar.open_time && std::isfinite(bar.open) &&
         std::isfinite(bar.high) && std::isfinite(bar.low) && std::isfinite(bar.close) && bar.high >= bar.low &&
         bar.volume >= 0;
}

/// @brief 校验订单回报是否可进入 Lane-T
[[nodiscard]] inline bool IsValidOrder(const qtrade_sdk::trader::Order& order) {
  return !(order.order_id.empty() && order.broker_order_id == 0 && order.client_order_id == 0) && order.volume >= 0 &&
         order.traded_volume >= 0 && order.left_volume >= 0 &&
         !(order.volume > 0 && order.traded_volume > order.volume);
}

/// @brief 校验成交回报是否可进入 Lane-T
[[nodiscard]] inline bool IsValidTrade(const qtrade_sdk::trader::Trade& trade) {
  return !trade.instrument.empty() && trade.volume > 0 && std::isfinite(trade.price) && trade.price >= 0.0 &&
         !(trade.order_id.empty() && trade.broker_order_id == 0 && trade.client_order_id == 0);
}

}  // namespace qtrade::common::utils

#endif  // QTRADE_COMMON_UTILS_ADAPTER_PAYLOAD_VALIDATION_HPP_
