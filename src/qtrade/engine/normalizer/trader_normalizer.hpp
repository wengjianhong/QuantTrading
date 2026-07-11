/// @file      trader_normalizer.hpp
/// @brief     交易标准化模块
/// @details   跨柜台订单/成交回报语义统一、校验过滤；标准化后交 OMS，再投递 Lane-R
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_
#define QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"

namespace qtrade::engine::normalizer {

class TraderNormalizer {
 public:
  explicit TraderNormalizer(event_bus::ReturnEventReactor& return_event_reactor);
  ~TraderNormalizer();

  void Start();
  void Stop();

 private:
  event_bus::ReturnEventReactor& return_event_reactor_;
  bool running_;
};

}  // namespace qtrade::engine::normalizer

#endif  // QTRADE_TRADING_ENGINE_NORMALIZER_TRADER_NORMALIZER_HPP_
