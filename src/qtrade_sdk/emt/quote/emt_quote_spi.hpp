/// @file      emt_quote_spi.hpp
/// @brief     EMT QuoteSpi 适配器（Adaptee 回调侧）
#ifndef QTRADE_ADAPTER_EMT_QUOTE_SPI_HPP_
#define QTRADE_ADAPTER_EMT_QUOTE_SPI_HPP_

#include <qtrade_sdk/quote/quote_spi.hpp>

namespace qtrade::adapter::quote {

/// 承接 EMT 行情回调，转换结构体后转发至 Target（qtrade_sdk::quote::QuoteSpi）。
/// TODO(EMT SDK): 改为 `final : public EMT::API::QuoteSpi`，在厂商回调 override 中完成转换。
class EmtQuoteSpi {
 public:
  void SetTarget(qtrade_sdk::quote::QuoteSpi* target) {
    target_ = target;
  }

  qtrade_sdk::quote::QuoteSpi* Target() const {
    return target_;
  }

 private:
  qtrade_sdk::quote::QuoteSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::quote

#endif  // QTRADE_ADAPTER_EMT_QUOTE_SPI_HPP_
