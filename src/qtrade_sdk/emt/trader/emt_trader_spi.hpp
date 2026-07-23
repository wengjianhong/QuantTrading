/// @file      emt_trader_spi.hpp
/// @brief     EMT TraderSpi 适配器（Adaptee 回调侧）
/// @details   承接 EMT 交易回调并转发给 qtrade SDK 回调接口。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_ADAPTER_EMT_TRADER_SPI_HPP_
#define QTRADE_ADAPTER_EMT_TRADER_SPI_HPP_

#include <qtrade_sdk/trader/trader_spi.hpp>

namespace qtrade::adapter::trader {

/// @brief EMT 交易 SPI 回调适配器。
/// @details TODO(EMT SDK): 改为 `final : public EMT::API::TraderSpi`，在厂商回调 override 中完成转换。
class EmtTraderSpi {
 public:
  /// @brief 设置接收转换后交易回调的目标对象。
  /// @param target qtrade SDK 交易回调目标；可为空。
  void SetTarget(qtrade_sdk::trader::TraderSpi* target) {
    target_ = target;
  }

  /// @brief 获取当前交易回调目标对象。
  /// @return 当前目标对象；未设置时为 nullptr。
  qtrade_sdk::trader::TraderSpi* Target() const {
    return target_;
  }

 private:
  /// @brief qtrade SDK 交易回调目标对象。
  qtrade_sdk::trader::TraderSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::trader

#endif  // QTRADE_ADAPTER_EMT_TRADER_SPI_HPP_
