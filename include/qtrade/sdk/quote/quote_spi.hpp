/// @file      quote_spi.hpp
/// @brief     行情回调接口
/// @details   参考 EMQ::API::QuoteSpi；业务数据使用 const 引用，可空字段保留指针。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_QUOTE_SPI_HPP_
#define QTRADE_SDK_QUOTE_SPI_HPP_

#include <qtrade/sdk/quote/quote_struct.hpp>

#include <cstdint>
#include <span>

namespace qtrade::sdk::quote {

/// @brief 行情回调接口。
class QuoteSpi {
 public:
  /// @brief 析构行情回调接口实例。
  virtual ~QuoteSpi() = default;

  /// @brief 错误应答。
  /// @param error_info 错误信息；为空表示无附加错误信息。
  virtual void OnError(const RspInfo* error_info) = 0;

  /// @brief 指数行情通知。
  /// @param index_data 指数行情数据。
  virtual void OnIndexData(const IndexData& index_data) = 0;

  /// @brief 深度行情通知，包含买一卖一队列。
  /// @param market_data 快照行情数据。
  /// @param bid1_qty 买一队列数量。
  /// @param ask1_qty 卖一队列数量。
  virtual void OnDepthMarketData(const MarketTick& market_data,
                                 std::span<const std::int64_t> bid1_qty,
                                 std::span<const std::int64_t> ask1_qty) = 0;

  /// @brief 订阅全市场指数行情应答。
  /// @param exchange_id 交易所代码。
  /// @param error_info 错误信息；为空表示无错误。
  virtual void OnSubscribeAllIndexData(ExchangeType exchange_id, const RspInfo* error_info) = 0;

  /// @brief 退订全市场指数行情应答。
  /// @param exchange_id 交易所代码。
  /// @param error_info 错误信息；为空表示无错误。
  virtual void OnUnSubscribeAllIndexData(ExchangeType exchange_id, const RspInfo* error_info) = 0;

  /// @brief 订阅单个或多个指数行情应答。
  /// @param ticker 订阅的指数代码。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次订阅最后一条回报。
  virtual void OnSubIndexData(const SpecificTicker& ticker, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 退订单个或多个指数行情应答。
  /// @param ticker 退订的指数代码。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次退订最后一条回报。
  virtual void OnUnSubIndexData(const SpecificTicker& ticker, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 订阅全市场快照行情应答。
  /// @param exchange_id 交易所代码。
  /// @param error_info 错误信息；为空表示无错误。
  virtual void OnSubscribeAllMarketData(ExchangeType exchange_id, const RspInfo* error_info) = 0;

  /// @brief 退订全市场快照行情应答。
  /// @param exchange_id 交易所代码。
  /// @param error_info 错误信息；为空表示无错误。
  virtual void OnUnSubscribeAllMarketData(ExchangeType exchange_id, const RspInfo* error_info) = 0;

  /// @brief 订阅快照行情应答。
  /// @param ticker 订阅的合约代码。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次订阅最后一条回报。
  virtual void OnSubMarketData(const SpecificTicker& ticker, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 退订快照行情应答。
  /// @param ticker 退订的合约代码。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次退订最后一条回报。
  virtual void OnUnSubMarketData(const SpecificTicker& ticker, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 查询全市场合约部分静态信息应答。
  /// @param qsi 合约部分静态信息。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次查询最后一条回报。
  virtual void OnQueryAllTickers(const QuoteStaticInfo& qsi, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 查询全市场合约完整静态信息应答。
  /// @param qfi 合约完整静态信息。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次查询最后一条回报。
  virtual void OnQueryAllTickersFullInfo(const QuoteFullInfo& qfi, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 查询最新指数行情应答。
  /// @param index_data 最新指数行情。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次查询最后一条回报。
  virtual void OnQueryLatestIndexData(const IndexData& index_data, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 查询最新快照行情应答。
  /// @param market_data 最新快照行情。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次查询最后一条回报。
  virtual void OnQueryLatestMarketData(const MarketTick& market_data, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 查询最新价应答。
  /// @param price_info 最新价信息。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次查询最后一条回报。
  virtual void OnQueryTickersPriceInfo(const TickerPriceInfo& price_info, const RspInfo* error_info, bool is_last) = 0;

  /// @brief 查询回补深市逐笔数据应答。
  /// @param rebuild_data 回补数据；为空表示仅错误通知。
  /// @param error_info 错误信息；为空表示无错误。
  /// @param is_last 是否为本次查询最后一条回报。
  virtual void OnRebuildSzData(const RebuildRespData* rebuild_data, const RspInfo* error_info, bool is_last) = 0;
};

}  // namespace qtrade::sdk::quote

#endif  // QTRADE_SDK_QUOTE_SPI_HPP_
