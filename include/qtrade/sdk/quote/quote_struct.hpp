/// @file      quote_struct.hpp
/// @brief     行情相关结构体
/// @details   参考 EMT_API 的行情结构体，聚合连接、订阅、快照、指数、静态信息与查询响应。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_QUOTE_STRUCT_HPP_
#define QTRADE_SDK_QUOTE_STRUCT_HPP_

#include <qtrade/sdk/constants/constants.hpp>
#include <qtrade/sdk/quote/quote_types.hpp>
#include <qtrade/sdk/trader/trader_types.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace qtrade_sdk::quote {

/// @brief API 错误响应信息，对应 EMTRspInfoStruct。
struct RspInfo {
  /// 错误代码；0 表示无错误。
  std::int32_t error_id = 0;
  /// 错误信息文本，长度参考 constants::kErrorMessageLength。
  std::string error_msg;
};

/// @brief 单个证券代码与交易所组合，对应 EMTSpecificTickerStruct。
struct SpecificTicker {
  /// 证券代码，不包含空格。
  std::string ticker;
  /// 证券所属交易所。
  ExchangeType exchange_id = ExchangeType::kUnknown;
};

/// @brief 行情连接请求。
struct ConnectRequest {
  /// 行情源名称，用于日志和监控区分。
  std::string name;
  /// 连接串，如 tcp://host:port。
  std::string connection_string;
  /// 登录用户名。
  std::string user;
  /// 登录密码。
  std::string password;
};

/// @brief 订阅行情请求。
struct SubscribeRequest {
  /// 合约代码列表；为空时由接口语义决定是否订阅全市场。
  std::vector<std::string> instruments;
  /// 交易所过滤条件。
  ExchangeType exchange = ExchangeType::kUnknown;
};

/// @brief 取消订阅行情请求。
struct UnsubscribeRequest {
  /// 合约代码列表。
  std::vector<std::string> instruments;
  /// 交易所过滤条件。
  ExchangeType exchange = ExchangeType::kUnknown;
};

/// @brief 查询最新快照请求。
struct QuerySnapshotRequest {
  /// 合约列表，空表示查询指定交易所全市场。
  std::vector<std::string> instruments;
  /// 证券类别。
  TickerType instrument_type = TickerType::kStock;
  /// 交易所代码。
  ExchangeType exchange = ExchangeType::kUnknown;
};

/// @brief K 线 / Bar 数据。
struct Bar {
  /// 合约代码。
  std::string instrument;
  /// 交易所。
  ExchangeType exchange = ExchangeType::kUnknown;
  /// 起始时间，格式 YYYYMMDDHHMMSSsss。
  std::int64_t open_time = 0;
  /// 结束时间，格式 YYYYMMDDHHMMSSsss。
  std::int64_t close_time = 0;
  /// 开盘价，单位元。
  double open = 0.0;
  /// 最高价，单位元。
  double high = 0.0;
  /// 最低价，单位元。
  double low = 0.0;
  /// 收盘价，单位元。
  double close = 0.0;
  /// 成交量，单位股/张。
  std::int64_t volume = 0;
  /// 成交金额，单位元。
  double turnover = 0.0;
  /// 均价，单位元。
  double avg_price = 0.0;
  /// 成交笔数。
  std::int64_t trades_count = 0;
  /// 持仓量，期货/期权品种使用。
  double open_interest = 0.0;
};

/// @brief 快照行情，对应 EMTMarketDataStruct 的核心字段。
struct MarketTick {
  /// 合约代码。
  std::string instrument;
  /// 交易所。
  ExchangeType exchange = ExchangeType::kUnknown;
  /// 证券类别。
  TickerType instrument_type = TickerType::kStock;
  /// 行情时间，格式 YYYYMMDDHHMMSSssss。
  std::int64_t data_time = 0;

  /// 最新价，单位元。
  double last_price = 0.0;
  /// 昨收价，单位元。
  double pre_close_price = 0.0;
  /// 开盘价，单位元。
  double open_price = 0.0;
  /// 最高价，单位元。
  double high_price = 0.0;
  /// 最低价，单位元。
  double low_price = 0.0;
  /// 收盘价，单位元。
  double close_price = 0.0;
  /// 涨停价，单位元。
  double upper_limit_price = 0.0;
  /// 跌停价，单位元。
  double lower_limit_price = 0.0;

  /// 总成交量，单位股/张。
  std::int64_t volume = 0;
  /// 成交笔数。
  std::int64_t trades_count = 0;
  /// 总成交金额，单位元。
  double turnover = 0.0;
  /// 当日均价，单位元。
  double avg_price = 0.0;
  /// 持仓量，期货/期权品种使用。
  double open_interest = 0.0;

  /// 申买价，下标 0 为买一。
  std::array<double, constants::kMaxOrderBookDepth> bid_price{};
  /// 申买量，下标 0 为买一。
  std::array<std::int64_t, constants::kMaxOrderBookDepth> bid_volume{};
  /// 申卖价，下标 0 为卖一。
  std::array<double, constants::kMaxOrderBookDepth> ask_price{};
  /// 申卖量，下标 0 为卖一。
  std::array<std::int64_t, constants::kMaxOrderBookDepth> ask_volume{};

  /// 委托买入总量。
  std::int64_t total_bid_volume = 0;
  /// 委托卖出总量。
  std::int64_t total_ask_volume = 0;
  /// 加权平均委买价。
  double ma_bid_price = 0.0;
  /// 加权平均委卖价。
  double ma_ask_price = 0.0;

  /// 交易状态说明。
  std::string ticker_status;
};

/// @brief 指数行情，对应 EMTIndexDataStruct 的核心字段。
struct IndexData {
  /// 指数代码。
  std::string ticker;
  /// 交易所。
  ExchangeType exchange_id = ExchangeType::kUnknown;
  /// 行情时间，格式 YYYYMMDDHHMMSSssss。
  std::int64_t data_time = 0;
  /// 最新指数点位。
  double last_price = 0.0;
  /// 昨收指数点位。
  double pre_close_price = 0.0;
  /// 开盘指数点位。
  double open_price = 0.0;
  /// 最高指数点位。
  double high_price = 0.0;
  /// 最低指数点位。
  double low_price = 0.0;
  /// 成交量。
  std::int64_t volume = 0;
  /// 成交金额。
  double turnover = 0.0;
};

/// @brief 逐笔成交。
struct TickByTickTrade {
  /// 合约代码。
  std::string instrument;
  /// 交易所。
  ExchangeType exchange = ExchangeType::kUnknown;
  /// 成交时间，格式 YYYYMMDDHHMMSSsss。
  std::int64_t data_time = 0;
  /// 成交价格，单位元。
  double price = 0.0;
  /// 成交数量。
  std::int64_t volume = 0;
  /// 成交金额，单位元。
  double turnover = 0.0;
  /// 内外盘方向。
  trader::SideType side = trader::SideType::kBuy;
  /// 逐笔序号。
  std::int64_t sequence = 0;
};

/// @brief 原始行情报文，主要用于适配器调试和审计留痕。
struct OriginalMarketTick {
  /// 数据源标识。
  std::string data_source;
  /// 原始报文内容。
  std::string raw_data;
};

/// @brief 合约部分静态信息，对应 EMTQuoteStaticInfo。
struct QuoteStaticInfo {
  /// 合约代码。
  std::string ticker;
  /// 合约名称。
  std::string ticker_name;
  /// 交易所。
  ExchangeType exchange_id = ExchangeType::kUnknown;
  /// 合约类型。
  TickerType ticker_type = TickerType::kUnknown;
  /// 昨收价。
  double pre_close_price = 0.0;
  /// 涨停价。
  double upper_limit_price = 0.0;
  /// 跌停价。
  double lower_limit_price = 0.0;
  /// 最小变动价位，按交易所原始精度保存。
  std::int64_t price_tick = 0;
};

/// @brief 合约完整静态信息，对应 EMTQuoteFullInfo 的常用字段。
struct QuoteFullInfo {
  /// 基础静态信息。
  QuoteStaticInfo static_info;
  /// 买入数量单位。
  std::int64_t buy_qty_unit = 0;
  /// 卖出数量单位。
  std::int64_t sell_qty_unit = 0;
  /// 合约乘数，现货通常为 1。
  std::int64_t volume_multiple = 0;
  /// 最小价格变动单位。
  double price_tick = 0.0;
  /// 是否注册制证券。
  bool is_registration = false;
  /// 是否 VIE 架构证券。
  bool is_vie = false;
  /// 是否尚未盈利证券。
  bool is_noprofit = false;
};

/// @brief 最新价查询结果，对应 EMTTickerPriceInfo。
struct TickerPriceInfo {
  /// 合约代码。
  std::string ticker;
  /// 交易所。
  ExchangeType exchange_id = ExchangeType::kUnknown;
  /// 最新价。
  double last_price = 0.0;
};

/// @brief 深市逐笔回补响应，对应 EMTRebuildRespData 的关键字段。
struct RebuildRespData {
  /// 回补通道号。
  std::uint32_t channel_no = 0;
  /// 回补起始序号。
  std::uint64_t begin_seq = 0;
  /// 回补结束序号。
  std::uint64_t end_seq = 0;
  /// 请求编号。
  std::uint64_t request_id = 0;
  /// 当前回补范围是否结束。
  bool finished = false;
};

/// @brief 查询快照响应。
struct QuerySnapshotResponse {
  /// 快照列表。
  std::vector<MarketTick> ticks;
};

}  // namespace qtrade_sdk::quote

#endif  // QTRADE_SDK_QUOTE_STRUCT_HPP_
