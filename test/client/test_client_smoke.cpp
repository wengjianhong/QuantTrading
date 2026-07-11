/// @file      client_smoke_test.cpp
/// @brief     客户端冒烟测试
/// @details   验证 LogClient、MonitorClient、OutboundWorker 可正常初始化与调用
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/client/log_client/log_client.hpp"
#include "qtrade/client/monitor_client/monitor_client.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

TEST(ClientSmoke, LogClientAsyncEmit) {
  qtrade::client::LogClient client;
  ASSERT_EQ(client.Init("smoke"), qtrade::ErrorCode::kSuccess);
  client.Emit("info", "client smoke");
  client.EmitAudit("order_id=1 action=send");
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  client.Shutdown();
}

TEST(ClientSmoke, MonitorClientAsyncMetrics) {
  qtrade::client::MonitorClient client;
  ASSERT_EQ(client.Init("stub://local"), qtrade::ErrorCode::kSuccess);
  client.Counter("test.counter", 1.0);
  client.Gauge("test.gauge", 42.0);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  client.Shutdown();
}
