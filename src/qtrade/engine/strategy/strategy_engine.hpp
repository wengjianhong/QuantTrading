/// @file      strategy_engine.hpp
/// @brief     策略引擎
/// @details   负责策略的注册、生命周期管理，接收市场数据并分发给策略
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_STRATEGY_ENGINE_HPP_
#define QTRADE_TRADING_ENGINE_STRATEGY_ENGINE_HPP_
#include "qtrade/engine/event_bus/event_lanes.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <functional>
#include <mutex>

namespace qtrade::engine::strategy {

using OrderSender = std::function<ErrorCode(const qtrade_sdk::trader::OrderRequest&)>;

class StrategyEngine {
 public:
  explicit StrategyEngine(event_bus::EventLanes& event_lanes);
  ~StrategyEngine();

  void Start();
  void Stop();

  void RegisterStrategy(std::unique_ptr<qtrade::strategy::IStrategy> strategy);
  void SetOrderSender(OrderSender sender);

 private:
  event_bus::EventLanes& event_lanes_;
  std::vector<std::unique_ptr<qtrade::strategy::IStrategy>> strategies_;
  /// Market / Return 双线程回调共用此锁，串行进入策略（最小锁；OMS 另有 mutex_）
  std::mutex mutex_;
  bool running_;
  OrderSender order_sender_;

  void OnTickEvent(const qtrade_sdk::quote::MarketTick& tick);
  void OnBarEvent(const qtrade_sdk::quote::Bar& bar);
  void OnOrderEvent(const qtrade_sdk::trader::Order& order);
  void OnTradeEvent(const qtrade_sdk::trader::Trade& trade);
};

}  // namespace qtrade::engine::strategy

#endif  // QTRADE_TRADING_ENGINE_STRATEGY_ENGINE_HPP_
