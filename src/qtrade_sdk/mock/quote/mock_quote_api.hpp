/// @file      mock_quote_api.hpp
/// @brief     Mock QuoteApi 适配器（Target: qtrade_sdk::quote::QuoteApi）
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

class MockQuoteApi final : public qtrade_sdk::quote::QuoteApi {
 public:
  MockQuoteApi();
  ~MockQuoteApi() override;

  void RegisterSpi(qtrade_sdk::quote::QuoteSpi& quote_spi) override;

  void UnregisterSpi() override;

  qtrade::ErrorCode Connect(const qtrade_sdk::quote::ConnectRequest& request) override;

  void Disconnect() override;

  bool IsConnected() const override;

  std::int32_t Login(const std::string& ip,
                     std::uint16_t port,
                     const std::string& user,
                     const std::string& pwd) override;

  void Logout() override;

  std::int32_t RebuildSzData(std::uint32_t channel_no,
                             std::uint64_t begin_seq,
                             std::uint64_t end_seq,
                             std::uint64_t request_id) override;

  void SetThreadAffinity(std::int32_t recv_cpu_no, std::int32_t process_cpu_no) override;

  std::int32_t SetBuffer(std::size_t buffer_size) override;

  int SubscribeAllIndexData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  int UnSubscribeAllIndexData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  int SubscribeIndexData(const std::vector<std::string>& tickers, qtrade_sdk::quote::ExchangeType exchange_id) override;

  int UnSubscribeIndexData(const std::vector<std::string>& tickers,
                           qtrade_sdk::quote::ExchangeType exchange_id) override;

  int SubscribeAllMarketData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  int UnSubscribeAllMarketData(qtrade_sdk::quote::ExchangeType exchange_id) override;

  int SubscribeMarketData(const std::vector<std::string>& tickers,
                          qtrade_sdk::quote::ExchangeType exchange_id) override;

  int UnSubscribeMarketData(const std::vector<std::string>& tickers,
                            qtrade_sdk::quote::ExchangeType exchange_id) override;

  qtrade::ErrorCode Subscribe(const qtrade_sdk::quote::SubscribeRequest& request) override;

  qtrade::ErrorCode Unsubscribe(const qtrade_sdk::quote::UnsubscribeRequest& request) override;

  int QueryAllTickers(qtrade_sdk::quote::ExchangeType exchange_id) override;

  int QueryAllTickersFullInfo(qtrade_sdk::quote::ExchangeType exchange_id) override;

  int QueryLatestInfo(const std::vector<std::string>& tickers,
                      qtrade_sdk::quote::TickerType ticker_type,
                      qtrade_sdk::quote::ExchangeType exchange_id) override;

  int QueryTickersPriceInfo(const std::vector<std::string>& tickers,
                            qtrade_sdk::quote::ExchangeType exchange_id) override;

  qtrade::ErrorCode QuerySnapshot(const qtrade_sdk::quote::QuerySnapshotRequest& request,
                                  qtrade_sdk::quote::QuerySnapshotResponse& response) override;

  void SetTickCallback(TickCallback cb) override;

  void SetBarCallback(BarCallback cb) override;

  std::vector<std::string> GetSupportedInstruments() const override;

 private:
  void GenerateMockTicks();

  MockQuoteSpi mock_spi_;
  qtrade_sdk::quote::QuoteSpi* quote_spi_ = nullptr;
  TickCallback on_tick_ = nullptr;
  BarCallback on_bar_ = nullptr;
  qtrade_sdk::quote::ConnectRequest config_;
  std::atomic<bool> connected_ = false;
  std::atomic<bool> running_ = false;
  std::vector<std::string> instruments_;
  std::mutex mutex_;
  std::thread tick_thread_;
};

/// @brief 创建 Mock 行情 Api，供 demo 与测试使用。
std::unique_ptr<qtrade_sdk::quote::QuoteApi> CreateMockQuoteApi();

}  // namespace qtrade::adapter::mock::quote

#endif  // QTRADE_ADAPTER_MOCK_QUOTE_API_HPP_
