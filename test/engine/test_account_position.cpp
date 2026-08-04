#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/position/position_manager.hpp"

#include <gtest/gtest.h>

TEST(AccountManager, TracksFrozenFundsAndDeduplicatesTrades) {
  qtrade::engine::account::AccountManager manager;

  qtrade_sdk::trader::AccountAsset asset;
  asset.buying_power = 1000.0;
  manager.ApplyAssetSnapshot(asset);

  qtrade_sdk::trader::Order order;
  order.order_id = "order-1";
  order.price = 10.0;
  order.volume = 10;
  order.left_volume = 10;
  order.side = qtrade_sdk::trader::SideType::kBuy;
  order.status = qtrade_sdk::trader::OrderStatusType::kNotTradedQueueing;
  manager.ApplyOrder(order);
  EXPECT_DOUBLE_EQ(manager.GetFrozenAmount(), 100.0);
  EXPECT_DOUBLE_EQ(manager.GetAvailableFunds(), 900.0);

  qtrade_sdk::trader::Trade trade;
  trade.trade_id = "trade-1";
  trade.order_id = order.order_id;
  trade.instrument = "IF2506";
  trade.price = 10.0;
  trade.volume = 4;
  trade.trade_amount = 40.0;
  trade.side = qtrade_sdk::trader::SideType::kBuy;
  manager.ApplyTrade(trade);
  manager.ApplyTrade(trade);

  order.left_volume = 6;
  order.traded_volume = 4;
  order.trade_amount = 40.0;
  order.status = qtrade_sdk::trader::OrderStatusType::kPartiallyFilled;
  manager.ApplyOrder(order);
  EXPECT_DOUBLE_EQ(manager.GetFilledAmount(), 40.0);
  EXPECT_DOUBLE_EQ(manager.GetNetCashFlow(), -40.0);
  EXPECT_DOUBLE_EQ(manager.GetFrozenAmount(), 60.0);
  EXPECT_DOUBLE_EQ(manager.GetAvailableFunds(), 900.0);
}

TEST(PositionManager, TracksLongShortAndTodayYesterday) {
  qtrade::engine::position::PositionManager manager;

  qtrade_sdk::trader::Position initial;
  initial.instrument = "IF2506";
  initial.total_volume = 5;
  initial.yesterday_volume = 5;
  initial.avg_price = 100.0;
  initial.direction = qtrade_sdk::trader::PositionDirectionType::kLong;
  manager.ApplyPositionSnapshot({initial});

  qtrade_sdk::trader::Trade open_long;
  open_long.trade_id = "trade-open-long";
  open_long.instrument = initial.instrument;
  open_long.price = 110.0;
  open_long.volume = 2;
  open_long.side = qtrade_sdk::trader::SideType::kBuy;
  open_long.position_effect = qtrade_sdk::trader::PositionEffectType::kOpen;
  manager.ApplyTrade(open_long);
  manager.ApplyTrade(open_long);
  EXPECT_EQ(manager.GetNetPosition(initial.instrument), 7);

  qtrade_sdk::trader::Trade close_yesterday;
  close_yesterday.trade_id = "trade-close-yesterday";
  close_yesterday.instrument = initial.instrument;
  close_yesterday.price = 111.0;
  close_yesterday.volume = 3;
  close_yesterday.side = qtrade_sdk::trader::SideType::kSell;
  close_yesterday.position_effect = qtrade_sdk::trader::PositionEffectType::kCloseYesterday;
  manager.ApplyTrade(close_yesterday);

  qtrade_sdk::trader::Trade close_today = close_yesterday;
  close_today.trade_id = "trade-close-today";
  close_today.volume = 1;
  close_today.position_effect = qtrade_sdk::trader::PositionEffectType::kCloseToday;
  manager.ApplyTrade(close_today);

  qtrade_sdk::trader::Trade open_short;
  open_short.trade_id = "trade-open-short";
  open_short.instrument = initial.instrument;
  open_short.price = 112.0;
  open_short.volume = 4;
  open_short.side = qtrade_sdk::trader::SideType::kSell;
  open_short.position_effect = qtrade_sdk::trader::PositionEffectType::kOpen;
  manager.ApplyTrade(open_short);

  const auto long_pos = manager.GetPosition(initial.instrument, qtrade_sdk::trader::PositionDirectionType::kLong);
  const auto short_pos = manager.GetPosition(initial.instrument, qtrade_sdk::trader::PositionDirectionType::kShort);
  ASSERT_TRUE(long_pos.has_value());
  ASSERT_TRUE(short_pos.has_value());
  EXPECT_EQ(long_pos->yesterday_volume, 2);
  EXPECT_EQ(long_pos->total_volume - long_pos->yesterday_volume, 1);
  EXPECT_EQ(short_pos->total_volume, 4);
  EXPECT_EQ(short_pos->total_volume - short_pos->yesterday_volume, 4);
  EXPECT_EQ(manager.GetNetPosition(initial.instrument), -1);
  EXPECT_EQ(manager.GetGrossPosition(initial.instrument), 7);
}
