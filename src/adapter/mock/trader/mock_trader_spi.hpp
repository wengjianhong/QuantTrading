/// @file      mock_trader_spi.hpp
/// @brief     Mock 交易 SPI（模拟厂商回调，转发至 Target）
#ifndef QTRADE_ADAPTER_MOCK_TRADER_SPI_HPP_
#define QTRADE_ADAPTER_MOCK_TRADER_SPI_HPP_

#include <qtrade_sdk/trader/trader_spi.hpp>

#include <cstdint>

namespace qtrade::adapter::mock::trader {

/// 模拟厂商 TraderSpi 回调侧，将合成回报转发至 qtrade_sdk::trader::TraderSpi。
class MockTraderSpi {
 public:
  void SetTarget(qtrade_sdk::trader::TraderSpi* target) {
    target_ = target;
  }

  qtrade_sdk::trader::TraderSpi* Target() const {
    return target_;
  }

  void PublishConnected();

  void PublishOrderEvent(const qtrade_sdk::trader::Order& order_info,
                         const qtrade_sdk::trader::RspInfo* error_info,
                         std::uint64_t session_id);

  void PublishTradeEvent(const qtrade_sdk::trader::Trade& trade_info, std::uint64_t session_id);

 private:
  qtrade_sdk::trader::TraderSpi* target_ = nullptr;
};

}  // namespace qtrade::adapter::mock::trader

#endif  // QTRADE_ADAPTER_MOCK_TRADER_SPI_HPP_
