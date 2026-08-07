/// @file      mock_quote_spi.hpp
/// @brief     Mock 行情 SPI（模拟厂商回调，转发至 Target）
/// @details   封装 Mock 行情事件并转发给 qtrade SDK 行情回调接口。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_ADAPTER_MOCK_QUOTE_SPI_HPP_
#define QTRADE_ADAPTER_MOCK_QUOTE_SPI_HPP_

#include <qtrade/sdk/quote/quote_spi.hpp>

namespace qtrade::adapter::mock::quote {

/// @brief Mock 行情 SPI 回调适配器。
/// @details 将合成行情转发至 qtrade SDK 行情回调接口。
class MockQuoteSpi {
 public:
  /// @brief 设置接收模拟行情回调的目标对象。
  /// @param target qtrade SDK 行情回调目标；可为空。
  void SetTarget(qtrade_sdk::quote::QuoteSpi* target) {
    target_ = target;
  }

  /// @brief 获取当前行情回调目标对象。
  /// @return 当前目标对象；未设置时为 nullptr。
  qtrade_sdk::quote::QuoteSpi* Target() const {
    return target_;
  }

  /// @brief 发布深度行情数据。
  /// @param market_data 待发布的行情 Tick 数据。
  void PublishDepthMarketData(const qtrade_sdk::quote::MarketTick& market_data);

 private:
  /// @brief qtrade SDK 行情回调目标对象。
  qtrade_sdk::quote::QuoteSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::mock::quote

#endif  // QTRADE_ADAPTER_MOCK_QUOTE_SPI_HPP_
