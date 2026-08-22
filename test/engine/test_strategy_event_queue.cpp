/// @file      test_strategy_event_queue.cpp
/// @brief     StrategyEventQueue：Lane 入队与策略回调线程隔离
#include "qtrade/engine/events/event_lanes.hpp"
#include "qtrade/engine/strategies/strategy_event_queue.hpp"
#include "qtrade/engine/strategies/strategy_manager.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
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

struct CallbackProbe {
  std::atomic<int> ticks{0};
  std::atomic<int> orders{0};
  std::atomic<int> concurrent{0};
  std::atomic<int> max_concurrent{0};
  std::atomic<std::thread::id> tick_thread{};
  std::chrono::milliseconds hold{0};
  std::atomic<bool> start_entered{false};
  std::chrono::milliseconds start_hold{0};
  std::atomic<int> last_tick_seq{0};
};

class ProbeStrategy final : public qtrade::strategy::IStrategy {
 public:
  explicit ProbeStrategy(CallbackProbe& probe) : probe_(probe) {}

  qtrade::ErrorCode Init(const qtrade::strategy::StrategyConfig&) override {
    return qtrade::ErrorCode::kSuccess;
  }
  qtrade::ErrorCode Start() override {
    probe_.start_entered.store(true, std::memory_order_release);
    if (probe_.start_hold > std::chrono::milliseconds::zero()) {
      std::this_thread::sleep_for(probe_.start_hold);
    }
    return qtrade::ErrorCode::kSuccess;
  }
  void Stop() override {}
  void SetOrderSender(qtrade::strategy::OrderSender) override {}
  qtrade::strategy::StrategyConfig GetStrategyConfig() const override {
    return {};
  }

  void OnTick(const qtrade::sdk::quote::MarketTick& tick) override {
    probe_.tick_thread.store(std::this_thread::get_id(), std::memory_order_release);
    probe_.last_tick_seq.store(static_cast<int>(tick.last_price), std::memory_order_release);
    const int now = probe_.concurrent.fetch_add(1, std::memory_order_acq_rel) + 1;
    int max = probe_.max_concurrent.load(std::memory_order_relaxed);
    while (now > max && !probe_.max_concurrent.compare_exchange_weak(max, now, std::memory_order_relaxed)) {
    }
    if (probe_.hold > std::chrono::milliseconds::zero()) {
      std::this_thread::sleep_for(probe_.hold);
    }
    probe_.concurrent.fetch_sub(1, std::memory_order_acq_rel);
    probe_.ticks.fetch_add(1, std::memory_order_relaxed);
  }
  void OnBar(const qtrade::sdk::quote::Bar&) override {}
  void OnOrder(const qtrade::sdk::trader::Order&) override {
    probe_.orders.fetch_add(1, std::memory_order_relaxed);
  }
  void OnTrade(const qtrade::sdk::trader::Trade&) override {}

 private:
  CallbackProbe& probe_;
};

[[nodiscard]] qtrade::engine::strategies::StrategyPtr MakeTestStrategyPtr(
  std::unique_ptr<qtrade::strategy::IStrategy> strategy) {
  return qtrade::engine::strategies::StrategyPtr{strategy.release(), [](qtrade::strategy::IStrategy* p) { delete p; }};
}

}  // namespace

TEST(StrategyEventQueue, EnqueueReturnsBeforeOnTickFinishes) {
  CallbackProbe probe;
  probe.hold = std::chrono::milliseconds(120);
  ProbeStrategy strategy(probe);
  qtrade::engine::strategies::StrategyEventQueue queue(strategy);
  queue.Start();

  qtrade::sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  const auto started = std::chrono::steady_clock::now();
  EXPECT_TRUE(queue.EnqueueTick(tick));
  const auto elapsed = std::chrono::steady_clock::now() - started;
  EXPECT_LT(elapsed, std::chrono::milliseconds(50));
  EXPECT_NE(std::this_thread::get_id(), probe.tick_thread.load(std::memory_order_acquire));

  WaitUntil([&] { return probe.ticks.load(std::memory_order_relaxed) == 1; }, std::chrono::milliseconds(500));
  EXPECT_EQ(probe.ticks.load(), 1);
  EXPECT_NE(std::this_thread::get_id(), probe.tick_thread.load(std::memory_order_acquire));

  queue.Stop();
}

TEST(StrategyEventQueue, SerializesOnTickOnOneWorker) {
  CallbackProbe probe;
  probe.hold = std::chrono::milliseconds(20);
  ProbeStrategy strategy(probe);
  qtrade::engine::strategies::StrategyEventQueue queue(strategy);
  queue.Start();

  qtrade::sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  EXPECT_TRUE(queue.EnqueueTick(tick));
  EXPECT_TRUE(queue.EnqueueTick(tick));

  WaitUntil([&] { return probe.ticks.load(std::memory_order_relaxed) == 2; }, std::chrono::milliseconds(500));
  EXPECT_EQ(probe.ticks.load(), 2);
  EXPECT_EQ(probe.max_concurrent.load(), 1);

  queue.Stop();
}

TEST(StrategyEventQueue, SlowStrategyDoesNotBlockOtherStrategyOnLaneQ) {
  qtrade::engine::events::EventLanes lanes;
  qtrade::engine::strategies::StrategyManager manager(lanes);

  CallbackProbe slow;
  slow.hold = std::chrono::milliseconds(200);
  CallbackProbe fast;
  ASSERT_EQ(manager.RegisterStrategy("slow", MakeTestStrategyPtr(std::make_unique<ProbeStrategy>(slow)), {"IF2506"}),
            qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(manager.RegisterStrategy("fast", MakeTestStrategyPtr(std::make_unique<ProbeStrategy>(fast)), {"IC2506"}),
            qtrade::ErrorCode::kSuccess);

  lanes.Start();
  ASSERT_EQ(manager.Start(), qtrade::ErrorCode::kSuccess);

  qtrade::sdk::quote::MarketTick slow_tick;
  slow_tick.instrument = "IF2506";
  lanes.Quote().PublishTick(slow_tick);
  WaitUntil([&] { return slow.concurrent.load(std::memory_order_acquire) == 1; }, std::chrono::milliseconds(200));
  ASSERT_EQ(slow.concurrent.load(), 1);

  qtrade::sdk::quote::MarketTick fast_tick;
  fast_tick.instrument = "IC2506";
  const auto started = std::chrono::steady_clock::now();
  lanes.Quote().PublishTick(fast_tick);
  WaitUntil([&] { return fast.ticks.load(std::memory_order_relaxed) == 1; }, std::chrono::milliseconds(150));
  const auto elapsed = std::chrono::steady_clock::now() - started;
  EXPECT_EQ(fast.ticks.load(), 1);
  EXPECT_LT(elapsed, std::chrono::milliseconds(150));
  EXPECT_EQ(slow.ticks.load(), 0);

  manager.SetDispatchActive(false);
  lanes.Stop();
  manager.Stop();
}

class ThrowingTickStrategy final : public qtrade::strategy::IStrategy {
 public:
  qtrade::ErrorCode Init(const qtrade::strategy::StrategyConfig&) override {
    return qtrade::ErrorCode::kSuccess;
  }
  qtrade::ErrorCode Start() override {
    return qtrade::ErrorCode::kSuccess;
  }
  void Stop() override {}
  void SetOrderSender(qtrade::strategy::OrderSender) override {}
  qtrade::strategy::StrategyConfig GetStrategyConfig() const override {
    return {};
  }

  void OnTick(const qtrade::sdk::quote::MarketTick&) override {
    const int n = ticks.fetch_add(1, std::memory_order_relaxed);
    if (n == 0) {
      throw 42;
    }
  }
  void OnBar(const qtrade::sdk::quote::Bar&) override {}
  void OnOrder(const qtrade::sdk::trader::Order&) override {}
  void OnTrade(const qtrade::sdk::trader::Trade&) override {}

  std::atomic<int> ticks{0};
};

TEST(StrategyEventQueue, DropsOldestTickWhenFull) {
  CallbackProbe probe;
  probe.hold = std::chrono::milliseconds(80);
  ProbeStrategy strategy(probe);
  qtrade::engine::strategies::StrategyEventQueue queue(strategy, 2);
  queue.Start();

  qtrade::sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  tick.last_price = 1;
  ASSERT_TRUE(queue.EnqueueTick(tick));
  WaitUntil([&] { return probe.concurrent.load(std::memory_order_acquire) == 1; }, std::chrono::milliseconds(200));

  tick.last_price = 2;
  ASSERT_TRUE(queue.EnqueueTick(tick));
  tick.last_price = 3;
  ASSERT_TRUE(queue.EnqueueTick(tick));
  tick.last_price = 4;
  ASSERT_TRUE(queue.EnqueueTick(tick));

  WaitUntil([&] { return probe.ticks.load(std::memory_order_relaxed) == 3; }, std::chrono::milliseconds(500));
  EXPECT_EQ(probe.ticks.load(), 3);
  EXPECT_EQ(probe.last_tick_seq.load(), 4);

  queue.Stop();
}

TEST(StrategyEventQueue, KeepsOrderWhenDroppingMarket) {
  CallbackProbe probe;
  probe.hold = std::chrono::milliseconds(80);
  ProbeStrategy strategy(probe);
  qtrade::engine::strategies::StrategyEventQueue queue(strategy, 2);
  queue.Start();

  qtrade::sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  tick.last_price = 1;
  ASSERT_TRUE(queue.EnqueueTick(tick));
  WaitUntil([&] { return probe.concurrent.load(std::memory_order_acquire) == 1; }, std::chrono::milliseconds(200));

  qtrade::sdk::trader::Order order;
  order.instrument = "IF2506";
  order.order_id = "keep-me";
  ASSERT_TRUE(queue.EnqueueOrder(order));
  tick.last_price = 2;
  ASSERT_TRUE(queue.EnqueueTick(tick));
  tick.last_price = 3;
  ASSERT_TRUE(queue.EnqueueTick(tick));

  WaitUntil([&] { return probe.ticks.load(std::memory_order_relaxed) >= 2 && probe.orders.load() == 1; },
            std::chrono::milliseconds(500));
  EXPECT_EQ(probe.orders.load(), 1);
  EXPECT_GE(probe.ticks.load(), 2);

  queue.Stop();
}

TEST(StrategyEventQueue, ContinuesAfterNonStdException) {
  ThrowingTickStrategy strategy;
  qtrade::engine::strategies::StrategyEventQueue queue(strategy);
  queue.Start();

  qtrade::sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  ASSERT_TRUE(queue.EnqueueTick(tick));
  ASSERT_TRUE(queue.EnqueueTick(tick));

  WaitUntil([&] { return strategy.ticks.load(std::memory_order_relaxed) == 2; }, std::chrono::milliseconds(500));
  EXPECT_EQ(strategy.ticks.load(), 2);

  queue.Stop();
}

TEST(StrategyManager, DoesNotDispatchUntilStrategyStartReturns) {
  qtrade::engine::events::EventLanes lanes;
  qtrade::engine::strategies::StrategyManager manager(lanes);

  CallbackProbe probe;
  probe.start_hold = std::chrono::milliseconds(120);
  ASSERT_EQ(manager.RegisterStrategy("s1", MakeTestStrategyPtr(std::make_unique<ProbeStrategy>(probe)), {"IF2506"}),
            qtrade::ErrorCode::kSuccess);

  lanes.Start();
  std::thread starter([&] { EXPECT_EQ(manager.Start(), qtrade::ErrorCode::kSuccess); });
  WaitUntil([&] { return probe.start_entered.load(std::memory_order_acquire); }, std::chrono::milliseconds(200));

  qtrade::sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  lanes.Quote().PublishTick(tick);
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  EXPECT_EQ(probe.ticks.load(), 0);

  starter.join();
  lanes.Quote().PublishTick(tick);
  WaitUntil([&] { return probe.ticks.load(std::memory_order_relaxed) == 1; }, std::chrono::milliseconds(500));
  EXPECT_EQ(probe.ticks.load(), 1);

  manager.SetDispatchActive(false);
  lanes.Stop();
  manager.Stop();
}

TEST(StrategyManager, SetDispatchActiveStopsLaneDelivery) {
  qtrade::engine::events::EventLanes lanes;
  qtrade::engine::strategies::StrategyManager manager(lanes);

  CallbackProbe probe;
  ASSERT_EQ(manager.RegisterStrategy("s1", MakeTestStrategyPtr(std::make_unique<ProbeStrategy>(probe)), {"IF2506"}),
            qtrade::ErrorCode::kSuccess);
  lanes.Start();
  ASSERT_EQ(manager.Start(), qtrade::ErrorCode::kSuccess);

  manager.SetDispatchActive(false);
  qtrade::sdk::quote::MarketTick tick;
  tick.instrument = "IF2506";
  lanes.Quote().PublishTick(tick);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(probe.ticks.load(), 0);

  manager.SetDispatchActive(true);
  lanes.Quote().PublishTick(tick);
  WaitUntil([&] { return probe.ticks.load(std::memory_order_relaxed) == 1; }, std::chrono::milliseconds(500));
  EXPECT_EQ(probe.ticks.load(), 1);

  manager.SetDispatchActive(false);
  lanes.Stop();
  manager.Stop();
}

TEST(StrategyManager, NotifyOrderBypassesDispatchGate) {
  qtrade::engine::events::EventLanes lanes;
  qtrade::engine::strategies::StrategyManager manager(lanes);

  CallbackProbe probe;
  ASSERT_EQ(manager.RegisterStrategy("s1", MakeTestStrategyPtr(std::make_unique<ProbeStrategy>(probe)), {"IF2506"}),
            qtrade::ErrorCode::kSuccess);
  lanes.Start();
  ASSERT_EQ(manager.Start(), qtrade::ErrorCode::kSuccess);
  manager.SetDispatchActive(false);

  qtrade::sdk::trader::Order order;
  order.instrument = "IF2506";
  order.order_id = "e-stage";
  manager.NotifyOrder(order);
  WaitUntil([&] { return probe.orders.load(std::memory_order_relaxed) == 1; }, std::chrono::milliseconds(500));
  EXPECT_EQ(probe.orders.load(), 1);

  lanes.Stop();
  manager.Stop();
}
