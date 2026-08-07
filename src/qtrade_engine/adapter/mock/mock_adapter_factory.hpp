/// @file      mock_adapter_factory.hpp
/// @brief     Mock 行情/交易适配器装配辅助
/// @details   构造 Mock QuoteApi/TraderApi，并填充连接请求；供 client / 单测调用。
/// @author    wengjianhong
/// @date      2026-08-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ADAPTER_MOCK_ADAPTER_FACTORY_HPP_
#define QTRADE_ADAPTER_MOCK_ADAPTER_FACTORY_HPP_

#include <qtrade/sdk/quote/quote_api.hpp>
#include <qtrade/sdk/quote/quote_struct.hpp>
#include <qtrade/sdk/trader/trader_api.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <memory>
#include <string>

namespace qtrade::adapter::mock {

/// @brief Mock 适配器与连接请求打包结果
struct MockAdapterBundle {
  /// 行情 API
  std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api;
  /// 交易 API
  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api;
  /// 行情连接请求
  qtrade_sdk::quote::ConnectRequest quote_request;
  /// 交易连接请求
  qtrade_sdk::trader::ConnectRequest trader_request;
};

/// @brief 构造 Mock 行情/交易适配器并填充连接请求
/// @param quote_connection_string 行情连接串（同时用作 mock 交易连接串）
/// @param account_id 交易账户号
/// @return 适配器与连接请求
[[nodiscard]] MockAdapterBundle CreateMockAdapters(const std::string& quote_connection_string,
                                                   const std::string& account_id);

}  // namespace qtrade::adapter::mock

#endif  // QTRADE_ADAPTER_MOCK_ADAPTER_FACTORY_HPP_
