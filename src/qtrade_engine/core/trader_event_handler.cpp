/// @file      trader_event_handler.cpp
/// @brief     Lane-T 引擎侧回报处理实现
/// @author    wengjianhong
/// @date      2026-08-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/trader_event_handler.hpp"

namespace qtrade::engine {

TraderEventHandler::TraderEventHandler(oms::OrderApi& orders,
                                       account::AccountApi& account,
                                       position::PositionApi& position,
                                       account_risk::AccountRiskApi& account_risk)
  : orders_(orders), account_(account), position_(position), account_risk_(account_risk) {}

void TraderEventHandler::Register(event_bus::EventLanes& event_lanes) {
  event_lanes.Trader().RegisterOrderCallback([this](const qtrade::sdk::trader::Order& order) { OnOrder(order); });
  event_lanes.Trader().RegisterTradeCallback([this](const qtrade::sdk::trader::Trade& trade) { OnTrade(trade); });
}

void TraderEventHandler::OnOrder(const qtrade::sdk::trader::Order& order) {
  orders_.ApplyOrderReport(order);
  const auto local_order =
    order.order_id.empty() ? orders_.GetOrderByClientId(order.client_order_id) : orders_.GetOrder(order.order_id);
  if (local_order.has_value()) {
    account_.ApplyOrder(*local_order);
  }

  if (order.status != qtrade::sdk::trader::OrderStatusType::kRejected &&
      order.status != qtrade::sdk::trader::OrderStatusType::kCanceled) {
    return;
  }
  if (!local_order.has_value()) {
    return;
  }
  const auto reason = order.status == qtrade::sdk::trader::OrderStatusType::kCanceled
                        ? qtrade::account_risk::ReleaseReason::kCanceled
                        : qtrade::account_risk::ReleaseReason::kRejectedByVenue;
  account_risk_.Release(local_order->order_id, reason);
}

void TraderEventHandler::OnTrade(const qtrade::sdk::trader::Trade& trade) {
  orders_.ApplyTradeReport(trade);
  account_.ApplyTrade(trade);
  position_.ApplyTrade(trade);

  const auto local_order =
    trade.order_id.empty() ? orders_.GetOrderByClientId(trade.client_order_id) : orders_.GetOrder(trade.order_id);
  if (local_order.has_value() && local_order->status == qtrade::sdk::trader::OrderStatusType::kFilled) {
    account_risk_.Release(local_order->order_id, qtrade::account_risk::ReleaseReason::kSettled);
  }
}

}  // namespace qtrade::engine
