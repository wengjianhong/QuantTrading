/// @file      emt_quote_spi.hpp
/// @brief     EMT QuoteSpi 适配器（Adaptee 回调侧）
/// @details   承接 EMT 行情回调并转发给 qtrade SDK 回调接口。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_ADAPTER_EMT_QUOTE_SPI_HPP_
#define QTRADE_ADAPTER_EMT_QUOTE_SPI_HPP_

#include <qtrade_sdk/quote/quote_spi.hpp>

namespace qtrade::adapter::quote {

/// @brief EMT 行情 SPI 回调适配器。
/// @details TODO(EMT SDK): 改为 `final : public EMT::API::QuoteSpi`，在厂商回调 override 中完成转换。
class EmtQuoteSpi {
 public:
  /// @brief 设置接收转换后行情回调的目标对象。
  /// @param target qtrade SDK 行情回调目标；可为空。
  void SetTarget(qtrade_sdk::quote::QuoteSpi* target) {
    target_ = target;
  }

  /// @brief 获取当前行情回调目标对象。
  /// @return 当前目标对象；未设置时为 nullptr。
  qtrade_sdk::quote::QuoteSpi* Target() const {
    return target_;
  }

 private:
  /// @brief qtrade SDK 行情回调目标对象。
  qtrade_sdk::quote::QuoteSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::quote

#endif  // QTRADE_ADAPTER_EMT_QUOTE_SPI_HPP_
