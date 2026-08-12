/// @file      stub_quote_spi.cpp
/// @brief     测试用 Stub 行情 SPI 实现
/// @details   将 Mock 生成的深度行情转发至已注册的标准行情 SPI。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "stubs/stub_quote_spi.hpp"

namespace qtrade::test::stub {

void StubQuoteSpi::PublishDepthMarketData(const qtrade::sdk::quote::MarketTick& market_data) {
  if (target_ != nullptr) {
    target_->OnDepthMarketData(market_data, {}, {});
  }
}

}  // namespace qtrade::test::stub
