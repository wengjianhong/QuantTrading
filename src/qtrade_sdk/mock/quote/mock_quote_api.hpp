/// @file      mock_quote_api.hpp
/// @brief     Mock QuoteApi 适配器（Target: qtrade_sdk::quote::QuoteApi）
/// @details   提供可用于演示和测试的内存行情接口实现。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright Copyright (c) 2026 qtrade.
#ifndef QTRADE_ADAPTER_MOCK_QUOTE_API_HPP_
#define QTRADE_ADAPTER_MOCK_QUOTE_API_HPP_

#include "qtrade_sdk/mock/quote/mock_quote_spi.hpp"

#include <qtrade_sdk/quote/quote_api.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace qtrade::adapter::mock::quote {

/// @brief Mock 行情 API 适配器。
/// @details 模拟行情连接、订阅和行情回调，实现 qtrade SDK 行情接口。
class MockQuoteApi final : public qtrade_sdk::quote::QuoteApi {
 public:
  /// @brief 构造 Mock 行情 API 适配器。
  MockQuoteApi();
  /// @brief 析构 Mock 行情 API 适配器。
  ~MockQuoteApi() override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::RegisterSpi
  void RegisterSpi(qtrade_sdk::quote::QuoteSpi& quote_spi) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::UnregisterSpi
  void UnregisterSpi() override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::Connect
  qtrade::ErrorCode Connect(const qtrade_sdk::quote::ConnectRequest& request) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::Disconnect
  void Disconnect() override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::IsConnected
  bool IsConnected() const override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::Login
  std::int32_t Login(const std::string& ip,
                     std::uint16_t port,
                     const std::string& user,
                     const std::string& pwd) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::Logout
  void Logout() override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::RebuildSzData
  std::int32_t RebuildSzData(std::uint32_t channel_no,
                             std::uint64_t begin_seq,
                             std::uint64_t end_seq,
                             std::uint64_t request_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SetThreadAffinity
  void SetThreadAffinity(std::int32_t recv_cpu_no, std::int32_t process_cpu_no) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SetBuffer
  std::int32_t SetBuffer(std::size_t buffer_size) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SubscribeAllIndexData
  int SubscribeAllIndexData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::UnSubscribeAllIndexData
  int UnSubscribeAllIndexData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SubscribeIndexData
  int SubscribeIndexData(const std::vector<std::string>& tickers, qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::UnSubscribeIndexData
  int UnSubscribeIndexData(const std::vector<std::string>& tickers,
                           qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SubscribeAllMarketData
  int SubscribeAllMarketData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::UnSubscribeAllMarketData
  int UnSubscribeAllMarketData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SubscribeMarketData
  int SubscribeMarketData(const std::vector<std::string>& tickers,
                          qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::UnSubscribeMarketData
  int UnSubscribeMarketData(const std::vector<std::string>& tickers,
                            qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::Subscribe
  qtrade::ErrorCode Subscribe(const qtrade_sdk::quote::SubscribeRequest& request) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::Unsubscribe
  qtrade::ErrorCode Unsubscribe(const qtrade_sdk::quote::UnsubscribeRequest& request) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::QueryAllTickers
  int QueryAllTickers(qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::QueryAllTickersFullInfo
  int QueryAllTickersFullInfo(qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::QueryLatestInfo
  int QueryLatestInfo(const std::vector<std::string>& tickers,
                      qtrade_sdk::quote::TickerType ticker_type,
                      qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::QueryTickersPriceInfo
  int QueryTickersPriceInfo(const std::vector<std::string>& tickers,
                            qtrade_sdk::quote::ExchangeType exchange_id) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::QuerySnapshot
  qtrade::ErrorCode QuerySnapshot(const qtrade_sdk::quote::QuerySnapshotRequest& request,
                                  qtrade_sdk::quote::QuerySnapshotResponse& response) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SetTickCallback
  void SetTickCallback(TickCallback cb) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::SetBarCallback
  void SetBarCallback(BarCallback cb) override;

  /// @copydoc qtrade_sdk::quote::QuoteApi::GetSupportedInstruments
  std::vector<std::string> GetSupportedInstruments() const override;

 private:
  /// @brief 生成并发布模拟 Tick 数据。
  void GenerateMockTicks();

  /// @brief Mock 行情回调适配器。
  MockQuoteSpi mock_spi_;
  /// @brief 已注册的 qtrade 行情回调对象。
  qtrade_sdk::quote::QuoteSpi* quote_spi_ = nullptr;
  /// @brief Tick 数据回调。
  TickCallback on_tick_ = nullptr;
  /// @brief Bar 数据回调。
  BarCallback on_bar_ = nullptr;
  /// @brief 当前连接配置。
  qtrade_sdk::quote::ConnectRequest config_;
  /// @brief 连接状态。
  std::atomic<bool> connected_ = false;
  /// @brief 模拟 Tick 生成线程的运行状态。
  std::atomic<bool> running_ = false;
  /// @brief 支持的合约列表。
  std::vector<std::string> instruments_;
  /// @brief 保护共享状态的互斥锁。
  std::mutex mutex_;
  /// @brief 生成模拟 Tick 的工作线程。
  std::thread tick_thread_;
};

/// @brief 创建 Mock 行情 API。
/// @return 用于演示与测试的行情 API 实例。
std::unique_ptr<qtrade_sdk::quote::QuoteApi> CreateMockQuoteApi();

}  // namespace qtrade::adapter::mock::quote

#endif  // QTRADE_ADAPTER_MOCK_QUOTE_API_HPP_
