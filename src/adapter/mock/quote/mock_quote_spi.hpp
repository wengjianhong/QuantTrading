/// @file      mock_quote_spi.hpp
/// @brief     Mock 行情 SPI（模拟厂商回调，转发至 Target）
#ifndef QTRADE_ADAPTER_MOCK_QUOTE_SPI_HPP_
#define QTRADE_ADAPTER_MOCK_QUOTE_SPI_HPP_

#include <qtrade_sdk/quote/quote_spi.hpp>

namespace qtrade::adapter::mock::quote {

/// 模拟厂商 QuoteSpi 回调侧，将合成行情转发至 qtrade_sdk::quote::QuoteSpi。
class MockQuoteSpi {
 public:
  void SetTarget(qtrade_sdk::quote::QuoteSpi* target) {
    target_ = target;
  }

  qtrade_sdk::quote::QuoteSpi* Target() const {
    return target_;
  }

  void PublishDepthMarketData(const qtrade_sdk::quote::MarketTick& market_data);

 private:
  qtrade_sdk::quote::QuoteSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::mock::quote

#endif  // QTRADE_ADAPTER_MOCK_QUOTE_SPI_HPP_
