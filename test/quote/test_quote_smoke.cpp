/// @file      test_quote_smoke.cpp
/// @brief     行情适配器冒烟测试
#include <qtrade_sdk/quote/quote_api.hpp>
#include <qtrade_sdk/quote/quote_spi.hpp>

#include <gtest/gtest.h>

TEST(QuoteSmoke, TypesCompile) {
  static_assert(true);
  SUCCEED();
}
