/// @file      trade_dedup.hpp
/// @brief     成交回报幂等键生成
/// @details   供 OMS / 持仓 / 账户等模块在 ApplyTrade 路径上去重；
///            不使用 price / volume / trade_time，避免同一笔成交因字段抖动重复入账。
/// @author    wengjianhong
/// @date      2026-08-04
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_UTILS_TRADE_DEDUP_HPP_
#define QTRADE_COMMON_UTILS_TRADE_DEDUP_HPP_

#include <qtrade/sdk/trader/trader_struct.hpp>

#include <string>

namespace qtrade::common::utils {

/// @brief 生成成交回报幂等键
/// @details 优先 trade_id（交易所/柜台成交编号）；若无，则拼接订单侧稳定标识与回报序号：
///          order_id、client_order_id、broker_order_id、exchange_order_id、report_index、instrument。
/// @param trade 成交回报
/// @return 用于 applied_trade_ids_ 等集合去重的键
inline std::string GenerateTradeDedupKey(const qtrade::sdk::trader::Trade& trade) {
  if (!trade.trade_id.empty()) {
    return trade.trade_id;
  }
  return trade.order_id + ":" + std::to_string(trade.client_order_id) + ":" + std::to_string(trade.broker_order_id) +
         ":" + trade.exchange_order_id + ":" + std::to_string(trade.report_index) + ":" + trade.instrument;
}

}  // namespace qtrade::common::utils

#endif  // QTRADE_COMMON_UTILS_TRADE_DEDUP_HPP_
