/// @file      test_trader_smoke.cpp
/// @brief     交易适配器冒烟测试
#include <qtrade_sdk/trader/trader_api.hpp>
#include <qtrade_sdk/trader/trader_spi.hpp>

#include <gtest/gtest.h>

TEST(TraderSmoke, TypesCompile) {
  static_assert(true);
  SUCCEED();
}
