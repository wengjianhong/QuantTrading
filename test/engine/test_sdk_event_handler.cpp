#include "qtrade/engine/core/quote_health_monitor.hpp"
#include "qtrade/engine/core/sdk_event_handler.hpp"
#include "qtrade/engine/event_bus/event_lanes.hpp"

#include <qtrade/sdk/trader/trader_struct.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace {

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

TEST(SdkEventHandler, PublishesValidOrderWhenRunning) {
  std::atomic<bool> running{true};
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::QuoteHealthMonitor health;
  qtrade::engine::SdkEventHandler handler(running, lanes, health);

  std::atomic<int> order_count{0};
  lanes.Trader().RegisterOrderCallback(
    [&](const qtrade::sdk::trader::Order&) { order_count.fetch_add(1, std::memory_order_relaxed); });
  lanes.Start();

  qtrade::sdk::trader::Order order;
  order.order_id = "ord-1";
  order.volume = 1;
  handler.OnOrder(order);

  WaitUntil([&] { return order_count.load(std::memory_order_relaxed) == 1; }, std::chrono::milliseconds(200));
  EXPECT_EQ(order_count.load(), 1);
  lanes.Stop();
}

TEST(SdkEventHandler, DropsWhenNotRunning) {
  std::atomic<bool> running{false};
  qtrade::engine::event_bus::EventLanes lanes;
  qtrade::engine::QuoteHealthMonitor health;
  qtrade::engine::SdkEventHandler handler(running, lanes, health);

  std::atomic<int> order_count{0};
  lanes.Trader().RegisterOrderCallback(
    [&](const qtrade::sdk::trader::Order&) { order_count.fetch_add(1, std::memory_order_relaxed); });
  lanes.Start();

  qtrade::sdk::trader::Order order;
  order.order_id = "ord-1";
  order.volume = 1;
  handler.OnOrder(order);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_EQ(order_count.load(), 0);
  lanes.Stop();
}
