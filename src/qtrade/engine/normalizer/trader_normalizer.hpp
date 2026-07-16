/// @file      trader_normalizer.hpp
/// @brief     交易标准化模块
/// @details   跨柜台订单/成交回报语义统一、校验过滤；标准化后交 OMS，再投递 Lane-R
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_
#define QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"

#include <qtrade_sdk/trader/trader_api.hpp>

#include <memory>

namespace qtrade::engine::normalizer {

class TraderNormalizer {
 public:
  explicit TraderNormalizer(event_bus::ReturnEventReactor& return_event_reactor);
  ~TraderNormalizer();

  void Start();
  void Stop();
  void SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api);
  qtrade_sdk::trader::TraderApi* GetTraderApi();

 private:
  void OnOrder(const qtrade_sdk::trader::Order& order);
  void OnTrade(const qtrade_sdk::trader::Trade& trade);

  event_bus::ReturnEventReactor& return_event_reactor_;
  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api_;
  bool running_ = false;
};

}  // namespace qtrade::engine::normalizer

#endif  // QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_
