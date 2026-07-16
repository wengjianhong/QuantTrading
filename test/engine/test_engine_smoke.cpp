/// @file      smoke_test.cpp
/// @brief     交易引擎冒烟测试
/// @details   验证 TradingEngine 启动/停止及 TickData 基本行为
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/config/qtrade_engine_config.hpp"
#include "qtrade/engine/trading_engine.hpp"

#include <qtrade_sdk/quote/quote_struct.hpp>

#include <gtest/gtest.h>

TEST(EngineSmoke, TradingEngineStartStop) {
  qtrade::common::config::QtradeEngineConfig config;
  config.identity.tenant_id = "test";
  config.identity.engine_id = "test-engine";
  config.identity.account_id = "test-account";
  config.support_services.config_service.enabled = false;
  config.support_services.account_risk_service.enabled = false;
  config.support_services.log_service.extensions["topic"] = "test";
  qtrade::engine::TradingEngine engine;
  ASSERT_EQ(engine.Init(config), qtrade::ErrorCode::kSuccess);
  ASSERT_EQ(engine.Start(), qtrade::ErrorCode::kSuccess);
  ASSERT_TRUE(engine.IsRunning());
  engine.Stop();
  ASSERT_FALSE(engine.IsRunning());
}

TEST(EngineSmoke, MarketTickSize) {
  qtrade_sdk::quote::MarketTick tick;
  (void)tick;
  SUCCEED();
}
