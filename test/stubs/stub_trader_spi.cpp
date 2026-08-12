/// @file      stub_trader_spi.cpp
/// @brief     测试用 Stub 交易 SPI 实现
/// @details   将 测试用 Stub 交易事件安全转发至已注册的标准交易 SPI。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "stubs/stub_trader_spi.hpp"

namespace qtrade::test::stub {

void StubTraderSpi::PublishConnected() {
  if (target_ != nullptr) {
    target_->OnConnected();
  }
}

void StubTraderSpi::PublishOrderEvent(const qtrade::sdk::trader::Order& order_info,
                                      const qtrade::sdk::trader::RspInfo* error_info,
                                      std::uint64_t session_id) {
  if (target_ != nullptr) {
    target_->OnOrderEvent(order_info, error_info, session_id);
  }
}

void StubTraderSpi::PublishTradeEvent(const qtrade::sdk::trader::Trade& trade_info, std::uint64_t session_id) {
  if (target_ != nullptr) {
    target_->OnTradeEvent(trade_info, session_id);
  }
}

}  // namespace qtrade::test::stub
