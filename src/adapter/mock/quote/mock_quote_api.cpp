/// @file      mock_quote_api.cpp
/// @brief     Mock QuoteApi 适配器实现
#include "adapter/mock/quote/mock_quote_api.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <random>

namespace qtrade::adapter::mock::quote {

namespace sdk = qtrade_sdk::quote;

MockQuoteApi::MockQuoteApi() = default;

MockQuoteApi::~MockQuoteApi() { Disconnect(); }

void MockQuoteApi::RegisterSpi(sdk::QuoteSpi& quote_spi) {
  quote_spi_ = &quote_spi;
  mock_spi_.SetTarget(&quote_spi);
}

void MockQuoteApi::UnregisterSpi() {
  quote_spi_ = nullptr;
  mock_spi_.SetTarget(nullptr);
}

qtrade::ErrorCode MockQuoteApi::Connect(const sdk::ConnectRequest& request) {
  if (connected_) {
    return qtrade::ErrorCode::kAlreadyConnected;
  }
  config_ = request;
  connected_ = true;
  running_ = true;
  tick_thread_ = std::thread([this]() { GenerateMockTicks(); });
  spdlog::info("[MockQuoteApi] connected: {}", config_.name);
  return qtrade::ErrorCode::kSuccess;
}

void MockQuoteApi::Disconnect() {
  if (!connected_) {
    return;
  }
  running_ = false;
  if (tick_thread_.joinable()) {
    tick_thread_.join();
  }
  connected_ = false;
  spdlog::info("[MockQuoteApi] disconnected");
}

bool MockQuoteApi::IsConnected() const { return connected_; }

std::int32_t MockQuoteApi::Login(const std::string& ip,
                                 std::uint16_t port,
                                 const std::string& user,
                                 const std::string& pwd) {
  (void)ip;
  (void)port;
  (void)user;
  (void)pwd;
  return 0;
}

void MockQuoteApi::Logout() { Disconnect(); }

std::int32_t MockQuoteApi::RebuildSzData(std::uint32_t channel_no,
                                         std::uint64_t begin_seq,
                                         std::uint64_t end_seq,
                                         std::uint64_t request_id) {
  (void)channel_no;
  (void)begin_seq;
  (void)end_seq;
  (void)request_id;
  return -1;
}

void MockQuoteApi::SetThreadAffinity(std::int32_t recv_cpu_no, std::int32_t process_cpu_no) {
  (void)recv_cpu_no;
  (void)process_cpu_no;
}

std::int32_t MockQuoteApi::SetBuffer(std::size_t buffer_size) {
  (void)buffer_size;
  return 0;
}

int MockQuoteApi::SubscribeAllIndexData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::UnSubscribeAllIndexData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::SubscribeIndexData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::UnSubscribeIndexData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::SubscribeAllMarketData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::UnSubscribeAllMarketData(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::SubscribeMarketData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::UnSubscribeMarketData(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

qtrade::ErrorCode MockQuoteApi::Subscribe(const sdk::SubscribeRequest& request) {
  std::lock_guard<std::mutex> lock(mutex_);
  instruments_ = request.instruments;
  spdlog::info("[MockQuoteApi] subscribed {} instruments", instruments_.size());
  return qtrade::ErrorCode::kSuccess;
}

qtrade::ErrorCode MockQuoteApi::Unsubscribe(const sdk::UnsubscribeRequest& request) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)request;
  instruments_.clear();
  return qtrade::ErrorCode::kSuccess;
}

int MockQuoteApi::QueryAllTickers(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::QueryAllTickersFullInfo(sdk::ExchangeType exchange_id) {
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::QueryLatestInfo(const std::vector<std::string>& tickers,
                                  sdk::TickerType ticker_type,
                                  sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)ticker_type;
  (void)exchange_id;
  return -1;
}

int MockQuoteApi::QueryTickersPriceInfo(const std::vector<std::string>& tickers, sdk::ExchangeType exchange_id) {
  (void)tickers;
  (void)exchange_id;
  return -1;
}

qtrade::ErrorCode MockQuoteApi::QuerySnapshot(const sdk::QuerySnapshotRequest& request,
                                              sdk::QuerySnapshotResponse& response) {
  (void)request;
  response.ticks.clear();
  return qtrade::ErrorCode::kNotSupported;
}

void MockQuoteApi::SetTickCallback(TickCallback cb) { on_tick_ = std::move(cb); }

void MockQuoteApi::SetBarCallback(BarCallback cb) { on_bar_ = std::move(cb); }

std::vector<std::string> MockQuoteApi::GetSupportedInstruments() const { return {"IF2401", "IC2401", "IH2401"}; }

void MockQuoteApi::GenerateMockTicks() {
  std::random_device rd;
  std::mt19937 gen(rd());
  double base_price = 100.0;

  while (running_) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (const auto& instrument : instruments_) {
        sdk::MarketTick tick;
        tick.instrument = instrument;
        tick.data_time =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
        base_price += std::uniform_real_distribution<>(-0.5, 0.5)(gen);
        tick.last_price = base_price;

        for (std::size_t i = 0; i < qtrade_sdk::constants::kDefaultOrderBookDepth; ++i) {
          tick.bid_price[i] = tick.last_price - 0.2 * static_cast<double>(i + 1);
          tick.bid_volume[i] = 100 + static_cast<int64_t>(i) * 50;
          tick.ask_price[i] = tick.last_price + 0.2 * static_cast<double>(i + 1);
          tick.ask_volume[i] = 100 + static_cast<int64_t>(i) * 50;
        }

        tick.volume = 1000;
        tick.turnover = tick.last_price * static_cast<double>(tick.volume);
        tick.avg_price = tick.last_price;
        tick.open_interest = 50000;
        tick.open_price = base_price - 1.0;
        tick.high_price = base_price + 2.0;
        tick.low_price = base_price - 2.0;
        tick.pre_close_price = base_price - 0.5;

        mock_spi_.PublishDepthMarketData(tick);

        if (on_tick_) {
          on_tick_(tick);
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

}  // namespace qtrade::adapter::mock::quote

namespace qtrade::adapter::mock::quote {

std::unique_ptr<qtrade_sdk::quote::QuoteApi> CreateMockQuoteApi() { return std::make_unique<MockQuoteApi>(); }

}  // namespace qtrade::adapter::mock::quote
