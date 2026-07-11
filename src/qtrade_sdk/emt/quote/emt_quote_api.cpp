/// @file      emt_quote_api.cpp
/// @brief     EMT QuoteApi 适配器实现
#include "qtrade_sdk/emt/quote/emt_quote_api.hpp"

namespace qtrade::adapter::quote {

namespace sdk = qtrade_sdk::quote;

EmtQuoteApi::EmtQuoteApi() = default;

EmtQuoteApi::~EmtQuoteApi() {
  Disconnect();
}

void EmtQuoteApi::RegisterSpi(sdk::QuoteSpi& quote_spi) {
  quote_spi_ = &quote_spi;
  emt_spi_.SetTarget(&quote_spi);
  // TODO(EMT SDK): emt_api_->RegisterSpi(&emt_spi_);
}

void EmtQuoteApi::UnregisterSpi() {
  quote_spi_ = nullptr;
  emt_spi_.SetTarget(nullptr);
  // TODO(EMT SDK): emt_api_->UnregisterSpi();
}

qtrade::ErrorCode EmtQuoteApi::Connect(const sdk::ConnectRequest& request) {
  if (connected_) {
    return qtrade::ErrorCode::kAlreadyConnected;
  }
  config_ = request;
  connected_ = true;
  // TODO(EMT SDK): 调用 emt_api_ 连接/登录
  return qtrade::ErrorCode::kNotSupported;
}

void EmtQuoteApi::Disconnect() {
  if (!connected_) {
    return;
  }
  connected_ = false;
  // TODO(EMT SDK): emt_api_->Logout(); emt_api_->Release();
}

bool EmtQuoteApi::IsConnected() const {
  return connected_;
}

std::int32_t EmtQuoteApi::Login(const std::string& ip,
                                std::uint16_t port,
                                const std::string& user,
                                const std::string& pwd) {
  (void)ip;
  (void)port;
  (void)user;
  (void)pwd;
  return -1;
}

void EmtQuoteApi::Logout() {
  Disconnect();
}

std::int32_t EmtQuoteApi::RebuildSzData(std::uint32_t channel_no,
                                        std::uint64_t begin_seq,
                                        std::uint64_t end_seq,
                                        std::uint64_t request_id) {
  (void)channel_no;
  (void)begin_seq;
  (void)end_seq;
  (void)request_id;
  return -1;
}

void EmtQuoteApi::SetThreadAffinity(std::int32_t recv_cpu_no, std::int32_t process_cpu_no) {
  (void)recv_cpu_no;
  (void)process_cpu_no;
}

std::int32_t EmtQuoteApi::SetBuffer(std::size_t buffer_size) {
  (void)buffer_size;
  return -1;
}

int EmtQuoteApi::SubscribeAllIndexData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::UnSubscribeAllIndexData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::SubscribeIndexData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::UnSubscribeIndexData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::SubscribeAllMarketData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::UnSubscribeAllMarketData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::SubscribeMarketData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::UnSubscribeMarketData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

qtrade::ErrorCode EmtQuoteApi::Subscribe(const sdk::SubscribeRequest& request) {
  std::lock_guard<std::mutex> lock(mutex_);
  instruments_ = request.instruments;
  // TODO(EMT SDK): emt_api_->SubscribeMarketData(...)
  return qtrade::ErrorCode::kNotSupported;
}

qtrade::ErrorCode EmtQuoteApi::Unsubscribe(const sdk::UnsubscribeRequest& request) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)request;
  instruments_.clear();
  return qtrade::ErrorCode::kNotSupported;
}

int EmtQuoteApi::QueryAllTickers(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::QueryAllTickersFullInfo(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::QueryLatestInfo(const std::vector<std::string>& tickers,
                                 sdk::TickerType ticker_type,
                                 sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)ticker_type;
  (void)exchange_id;
  return -1;
}

int EmtQuoteApi::QueryTickersPriceInfo(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

qtrade::ErrorCode EmtQuoteApi::QuerySnapshot(const sdk::QuerySnapshotRequest& request,
                                             sdk::QuerySnapshotResponse& response) {
  (void)request;
  response.ticks.clear();
  return qtrade::ErrorCode::kNotSupported;
}

void EmtQuoteApi::SetTickCallback(TickCallback cb) {
  on_tick_ = std::move(cb);
}

void EmtQuoteApi::SetBarCallback(BarCallback cb) {
  on_bar_ = std::move(cb);
}

std::vector<std::string> EmtQuoteApi::GetSupportedInstruments() const {
  return {};
}

}  // namespace qtrade::adapter::quote
