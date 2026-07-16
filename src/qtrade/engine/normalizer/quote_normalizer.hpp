/// @file      quote_normalizer.hpp
/// @brief     行情标准化模块
/// @details   跨柜台行情语义统一、校验过滤；标准化后投递 Lane-M（队列由 EventBus 承担）
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_NORMALIZER_QUOTE_NORMALIZER_HPP_
#define QTRADE_TRADING_ENGINE_NORMALIZER_QUOTE_NORMALIZER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"

#include <qtrade_sdk/quote/quote_api.hpp>

#include <memory>
#include <mutex>
#include <vector>

namespace qtrade::engine::normalizer {

class QuoteNormalizer {
 public:
  explicit QuoteNormalizer(event_bus::MarketEventReactor& market_event_reactor);
  ~QuoteNormalizer();

  void Start();
  void Stop();

  void SetQuoteApi(std::unique_ptr<qtrade_sdk::quote::QuoteApi> source);
  qtrade_sdk::quote::QuoteApi* GetQuoteApi();

  void Subscribe(const std::vector<std::string>& instruments);
  void Unsubscribe(const std::vector<std::string>& instruments);

 private:
  event_bus::MarketEventReactor& market_event_reactor_;
  std::unique_ptr<qtrade_sdk::quote::QuoteApi> market_source_;
  std::mutex mutex_;
  bool running_ = false;

  void OnTick(const qtrade_sdk::quote::MarketTick& tick);
  void OnBar(const qtrade_sdk::quote::Bar& bar);
};

}  // namespace qtrade::engine::normalizer

#endif  // QTRADE_TRADING_ENGINE_NORMALIZER_QUOTE_NORMALIZER_HPP_
