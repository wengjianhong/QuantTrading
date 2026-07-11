/// @file      mock_trader_spi.cpp
/// @brief     Mock 交易 SPI 实现
#include "qtrade_sdk/mock/trader/mock_trader_spi.hpp"

namespace qtrade::adapter::mock::trader {

void MockTraderSpi::PublishConnected() {
  if (target_ != nullptr) {
    target_->OnConnected();
  }
}

void MockTraderSpi::PublishOrderEvent(const qtrade_sdk::trader::Order& order_info,
                                      const qtrade_sdk::trader::RspInfo* error_info,
                                      std::uint64_t session_id) {
  if (target_ != nullptr) {
    target_->OnOrderEvent(order_info, error_info, session_id);
  }
}

void MockTraderSpi::PublishTradeEvent(const qtrade_sdk::trader::Trade& trade_info, std::uint64_t session_id) {
  if (target_ != nullptr) {
    target_->OnTradeEvent(trade_info, session_id);
  }
}

}  // namespace qtrade::adapter::mock::trader
