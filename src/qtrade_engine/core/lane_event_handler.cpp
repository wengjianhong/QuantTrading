/// @file      lane_event_handler.cpp
/// @brief     Lane-T 引擎侧回报处理实现
/// @author    wengjianhong
/// @date      2026-08-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/lane_event_handler.hpp"

namespace qtrade::engine {
using qtrade::account_risk::ReleaseReason;
using qtrade::sdk::trader::Order;
using qtrade::sdk::trader::OrderStatusType;
using qtrade::sdk::trader::Trade;

LaneEventHandler::LaneEventHandler(orders::OrderApi& orders,
                                   account::AccountApi& account,
                                   positions::PositionApi& position,
                                   account_risk::AccountRiskApi& account_risk)
  : orders_api_(orders), account_api_(account), position_api_(position), account_risk_api_(account_risk) {}

void LaneEventHandler::Register(events::EventLanes& event_lanes) {
  event_lanes.Trader().RegisterOrderCallback([this](const Order& order) { OnOrder(order); });
  event_lanes.Trader().RegisterTradeCallback([this](const Trade& trade) { OnTrade(trade); });
}

void LaneEventHandler::OnOrder(const Order& order) {
  orders_api_.ApplyOrderReport(order);
  const auto local_order = order.order_id.empty() ? orders_api_.GetOrderByClientId(order.client_order_id)
                                                  : orders_api_.GetOrder(order.order_id);
  if (local_order.has_value()) {
    account_api_.ApplyOrder(*local_order);
  }

  if (order.status != OrderStatusType::kRejected && order.status != OrderStatusType::kCanceled) {
    return;
  }
  if (!local_order.has_value()) {
    return;
  }
  const auto reason =
    order.status == OrderStatusType::kCanceled ? ReleaseReason::kCanceled : ReleaseReason::kRejectedByVenue;
  account_risk_api_.Release(local_order->order_id, reason);
}

void LaneEventHandler::OnTrade(const Trade& trade) {
  orders_api_.ApplyTradeReport(trade);
  account_api_.ApplyTrade(trade);
  position_api_.ApplyTrade(trade);

  const auto local_order = trade.order_id.empty() ? orders_api_.GetOrderByClientId(trade.client_order_id)
                                                  : orders_api_.GetOrder(trade.order_id);
  if (local_order.has_value() && local_order->status == OrderStatusType::kFilled) {
    account_risk_api_.Release(local_order->order_id, ReleaseReason::kSettled);
  }
}

}  // namespace qtrade::engine
