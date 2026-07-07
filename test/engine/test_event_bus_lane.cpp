/// @file      test_event_bus_lane.cpp
/// @brief     EventLanes 双线程独立分发测试
/// @author    wengjianhong
/// @date      2026-06-25
/// @copyright CC BY-NC-SA 4.0

#include <gtest/gtest.h>

#include <qtrade_sdk/quote/quote_struct.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include "engine/event_bus/event_lanes.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace {

using qtrade::engine::event_bus::EventLanes;

void WaitUntil(const std::function<bool()>& pred, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

}  // namespace

TEST(EventBusLane, ReturnRunsWhileMarketCallbackBlocked) {
  EventLanes lanes;
  std::atomic<bool> market_in_callback{false};
  std::atomic<int> order_count{0};

  lanes.Market().SubscribeTick([&](const qtrade_sdk::quote::MarketTick&) {
    market_in_callback.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    market_in_callback.store(false, std::memory_order_release);
  });
  lanes.Return().SubscribeOrder(
    [&](const qtrade_sdk::trader::Order&) { order_count.fetch_add(1, std::memory_order_relaxed); });

  lanes.Start();

  qtrade_sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  lanes.Market().PublishTick(tick);

  WaitUntil([&] { return market_in_callback.load(std::memory_order_acquire); }, std::chrono::milliseconds(200));

  qtrade_sdk::trader::Order order;
  order.order_id = "ord-1";
  lanes.Return().PublishOrder(order);

  WaitUntil([&] { return order_count.load(std::memory_order_relaxed) == 1; }, std::chrono::milliseconds(200));

  EXPECT_EQ(order_count.load(), 1);

  lanes.Stop();
}

TEST(EventBusLane, ReturnCompletesBeforeAllMarketTicksFinished) {
  EventLanes lanes;
  std::atomic<int> ticks_finished{0};
  std::atomic<int> order_count{0};
  std::atomic<int> ticks_done_at_order{-1};

  lanes.Market().SubscribeTick([&](const qtrade_sdk::quote::MarketTick&) {
    if (ticks_finished.load(std::memory_order_relaxed) == 0) {
      qtrade_sdk::trader::Order order;
      order.order_id = "ord-inject";
      lanes.Return().PublishOrder(order);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ticks_finished.fetch_add(1, std::memory_order_relaxed);
  });
  lanes.Return().SubscribeOrder([&](const qtrade_sdk::trader::Order&) {
    ticks_done_at_order.store(ticks_finished.load(std::memory_order_relaxed), std::memory_order_relaxed);
    order_count.fetch_add(1, std::memory_order_relaxed);
  });

  lanes.Start();

  for (int i = 0; i < 3; ++i) {
    qtrade_sdk::quote::MarketTick tick;
    tick.instrument = "IC2506";
    lanes.Market().PublishTick(tick);
  }

  WaitUntil(
    [&] {
      return ticks_finished.load(std::memory_order_relaxed) == 3 && order_count.load(std::memory_order_relaxed) == 1;
    },
    std::chrono::milliseconds(1000));

  ASSERT_EQ(order_count.load(), 1);
  ASSERT_EQ(ticks_finished.load(), 3);
  EXPECT_LT(ticks_done_at_order.load(), 3);

  lanes.Stop();
}
