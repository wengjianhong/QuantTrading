/// @file      quote_api.hpp
/// @brief     行情 API 抽象接口
/// @details   参考 EMQ::API::QuoteApi，所有接口均为纯虚函数。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_QUOTE_API_HPP_
#define QTRADE_SDK_QUOTE_API_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/quote/quote_spi.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace qtrade::sdk::quote {

/// @brief 行情 API 抽象接口。
class QuoteApi {
 public:
  /// @brief Tick 推送回调类型。
  using TickCallback = std::function<void(const MarketTick&)>;
  /// @brief Bar 推送回调类型。
  using BarCallback = std::function<void(const Bar&)>;

  /// @brief 析构行情 API 接口实例。
  virtual ~QuoteApi() = default;

  /// @brief 注册行情回调接口；应在登录前调用。
  /// @param quote_spi 行情 SPI 实现对象。
  virtual void RegisterSpi(QuoteSpi& quote_spi) = 0;

  /// @brief 取消注册行情回调接口。
  virtual void UnregisterSpi() = 0;

  /// @brief 项目内部行情源连接入口。
  /// @param request 连接请求参数。
  /// @return 连接结果错误码。
  virtual qtrade::ErrorCode Connect(const ConnectRequest& request) = 0;

  /// @brief 项目内部行情源断开入口。
  virtual void Disconnect() = 0;

  /// @brief 返回当前连接状态。
  /// @return 已连接返回 true，否则返回 false。
  virtual bool IsConnected() const = 0;

  /// @brief 用户登录请求。
  /// @param ip 行情服务器地址。
  /// @param port 行情服务器端口。
  /// @param user 登录用户名。
  /// @param pwd 登录密码。
  /// @return 0 表示成功，非 0 表示失败。
  virtual std::int32_t Login(const std::string& ip,
                             std::uint16_t port,
                             const std::string& user,
                             const std::string& pwd) = 0;

  /// @brief 登出请求。
  virtual void Logout() = 0;

  /// @brief 查询回补深市逐笔数据。
  /// @param channel_no 回补数据通道号。
  /// @param begin_seq 回补起始序号。
  /// @param end_seq 回补结束序号。
  /// @param request_id 用户自定义请求编号。
  /// @return 0 表示请求发送成功，非 0 表示失败。
  virtual std::int32_t RebuildSzData(std::uint32_t channel_no,
                                     std::uint64_t begin_seq,
                                     std::uint64_t end_seq,
                                     std::uint64_t request_id) = 0;

  /// @brief 设置接收与处理线程 CPU 亲和。
  /// @param recv_cpu_no 接收线程绑定 CPU；-1 表示不绑核。
  /// @param process_cpu_no 处理线程绑定 CPU；-1 表示不绑核。
  virtual void SetThreadAffinity(std::int32_t recv_cpu_no = -1, std::int32_t process_cpu_no = -1) = 0;

  /// @brief 设置 API 内部消息缓冲区大小。
  /// @param buffer_size 缓冲区大小，单位 MB。
  /// @return 0 表示成功，非 0 表示失败。
  virtual std::int32_t SetBuffer(std::size_t buffer_size) = 0;

  /// @brief 订阅全市场指数快照行情。
  /// @param exchange_id 交易所过滤条件。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int SubscribeAllIndexData(ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 取消订阅全市场指数快照行情。
  /// @param exchange_id 交易所过滤条件。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int UnSubscribeAllIndexData(ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 订阅指数快照行情。
  /// @param tickers 指数代码列表。
  /// @param exchange_id 交易所代码。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int SubscribeIndexData(const std::vector<std::string>& tickers,
                                 ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 取消订阅指数快照行情。
  /// @param tickers 指数代码列表。
  /// @param exchange_id 交易所代码。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int UnSubscribeIndexData(const std::vector<std::string>& tickers,
                                   ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 订阅全市场合约快照行情。
  /// @param exchange_id 交易所过滤条件。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int SubscribeAllMarketData(ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 退订全市场合约快照行情。
  /// @param exchange_id 交易所过滤条件。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int UnSubscribeAllMarketData(ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 订阅快照行情，包括股票、基金、债券、权证和质押式回购。
  /// @param tickers 合约代码列表。
  /// @param exchange_id 交易所代码。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int SubscribeMarketData(const std::vector<std::string>& tickers,
                                  ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 取消订阅快照行情，包括股票、基金、债券、权证和质押式回购。
  /// @param tickers 合约代码列表。
  /// @param exchange_id 交易所代码。
  /// @return 0 表示接口调用成功，非 0 表示失败。
  virtual int UnSubscribeMarketData(const std::vector<std::string>& tickers,
                                    ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 项目内部统一订阅入口。
  /// @param request 订阅请求参数。
  /// @return 订阅结果错误码。
  virtual qtrade::ErrorCode Subscribe(const SubscribeRequest& request) = 0;

  /// @brief 项目内部统一退订入口。
  /// @param request 退订请求参数。
  /// @return 退订结果错误码。
  virtual qtrade::ErrorCode Unsubscribe(const UnsubscribeRequest& request) = 0;

  /// @brief 获取所有合约的部分静态信息。
  /// @param exchange_id 交易所过滤条件。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryAllTickers(ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 获取所有合约的完整静态信息。
  /// @param exchange_id 交易所过滤条件。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryAllTickersFullInfo(ExchangeType exchange_id = ExchangeType::kUnknown) = 0;

  /// @brief 获取最新快照信息。
  /// @param tickers 合约代码列表；空表示查询全市场。
  /// @param ticker_type 证券类别。
  /// @param exchange_id 交易所代码。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryLatestInfo(const std::vector<std::string>& tickers,
                              TickerType ticker_type,
                              ExchangeType exchange_id) = 0;

  /// @brief 获取最新价。
  /// @param tickers 合约代码列表；空表示查询全市场。
  /// @param exchange_id 交易所代码。
  /// @return 0 表示查询请求发送成功，非 0 表示失败。
  virtual int QueryTickersPriceInfo(const std::vector<std::string>& tickers, ExchangeType exchange_id) = 0;

  /// @brief 项目内部统一快照查询入口。
  /// @param request 查询请求参数。
  /// @param response 查询结果输出。
  /// @return 查询结果错误码。
  virtual qtrade::ErrorCode QuerySnapshot(const QuerySnapshotRequest& request, QuerySnapshotResponse& response) = 0;

  /// @brief 注册 Tick 推送回调。
  /// @param cb Tick 推送回调函数。
  virtual void SetTickCallback(TickCallback cb) = 0;

  /// @brief 注册 Bar 推送回调。
  /// @param cb Bar 推送回调函数。
  virtual void SetBarCallback(BarCallback cb) = 0;

  /// @brief 返回适配器支持的合约列表。
  /// @return 支持的合约代码列表。
  virtual std::vector<std::string> GetSupportedInstruments() const = 0;
};

}  // namespace qtrade::sdk::quote

#endif  // QTRADE_SDK_QUOTE_API_HPP_
