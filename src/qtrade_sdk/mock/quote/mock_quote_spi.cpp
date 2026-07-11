/// @file      mock_quote_spi.cpp
/// @brief     Mock 行情 SPI 实现
#include "qtrade_sdk/mock/quote/mock_quote_spi.hpp"

namespace qtrade::adapter::mock::quote {

void MockQuoteSpi::PublishDepthMarketData(const qtrade_sdk::quote::MarketTick& market_data) {
  if (target_ != nullptr) {
    target_->OnDepthMarketData(market_data, {}, {});
  }
}

}  // namespace qtrade::adapter::mock::quote
