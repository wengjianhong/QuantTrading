/// @file      mock_adapter_factory.cpp
/// @brief     Mock 行情/交易适配器装配实现
/// @author    wengjianhong
/// @date      2026-08-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade_sdk/mock/mock_adapter_factory.hpp"

#include "qtrade_sdk/mock/quote/mock_quote_api.hpp"
#include "qtrade_sdk/mock/trader/mock_trader_api.hpp"

namespace qtrade::adapter::mock {

MockAdapterBundle CreateMockAdapters(const std::string& quote_connection_string, const std::string& account_id) {
  MockAdapterBundle bundle;
  bundle.quote_request.name = "mock";
  bundle.quote_request.connection_string = quote_connection_string;
  bundle.trader_request.broker_id = "mock";
  bundle.trader_request.account_id = account_id;
  bundle.trader_request.connection_string = quote_connection_string;
  bundle.quote_api = quote::CreateMockQuoteApi();
  bundle.trader_api = trader::CreateMockTraderApi();
  return bundle;
}

}  // namespace qtrade::adapter::mock
