/// @file      stub_quote_spi.hpp
/// @brief     测试用 Stub 行情 SPI（模拟厂商回调，转发至 Target）
/// @details   封装 测试用 Stub 行情事件并转发给 qtrade SDK 行情回调接口。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_TEST_STUB_QUOTE_SPI_HPP_
#define QTRADE_TEST_STUB_QUOTE_SPI_HPP_

#include <qtrade/sdk/quote/quote_spi.hpp>

namespace qtrade::test::stub {

/// @brief 测试用 Stub 行情 SPI 回调适配器。
/// @details 将合成行情转发至 qtrade SDK 行情回调接口。
class StubQuoteSpi {
 public:
  /// @brief 设置接收模拟行情回调的目标对象。
  /// @param target qtrade SDK 行情回调目标；可为空。
  void SetTarget(qtrade::sdk::quote::QuoteSpi* target) {
    target_ = target;
  }

  /// @brief 获取当前行情回调目标对象。
  /// @return 当前目标对象；未设置时为 nullptr。
  qtrade::sdk::quote::QuoteSpi* Target() const {
    return target_;
  }

  /// @brief 发布深度行情数据。
  /// @param market_data 待发布的行情 Tick 数据。
  void PublishDepthMarketData(const qtrade::sdk::quote::MarketTick& market_data);

 private:
  /// @brief qtrade SDK 行情回调目标对象。
  qtrade::sdk::quote::QuoteSpi* target_ = nullptr;
};

}  // namespace qtrade::test::stub

#endif  // QTRADE_TEST_STUB_QUOTE_SPI_HPP_
