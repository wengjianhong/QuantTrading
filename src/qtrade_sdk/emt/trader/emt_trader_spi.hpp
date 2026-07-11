/// @file      emt_trader_spi.hpp
/// @brief     EMT TraderSpi 适配器（Adaptee 回调侧）
#ifndef QTRADE_ADAPTER_EMT_TRADER_SPI_HPP_
#define QTRADE_ADAPTER_EMT_TRADER_SPI_HPP_

#include <qtrade_sdk/trader/trader_spi.hpp>

namespace qtrade::adapter::trader {

/// 承接 EMT 交易回调，转换结构体后转发至 Target（qtrade_sdk::trader::TraderSpi）。
/// TODO(EMT SDK): 改为 `final : public EMT::API::TraderSpi`，在厂商回调 override 中完成转换。
class EmtTraderSpi {
 public:
  void SetTarget(qtrade_sdk::trader::TraderSpi* target) {
    target_ = target;
  }

  qtrade_sdk::trader::TraderSpi* Target() const {
    return target_;
  }

 private:
  qtrade_sdk::trader::TraderSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::trader

#endif  // QTRADE_ADAPTER_EMT_TRADER_SPI_HPP_
