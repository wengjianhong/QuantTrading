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
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::strategy {

/// @brief 策略发单回调类型
using OrderSender = std::function<ErrorCode(const qtrade_sdk::trader::OrderRequest&)>;

/// @brief 策略注册、行情/回报分发与发单回调桥接
class StrategyEngine {
 public:
  /// @brief 构造策略引擎并绑定事件通道
  /// @param event_lanes Lane-M / Lane-R 事件门面
  explicit StrategyEngine(event_bus::EventLanes& event_lanes);

  /// @brief 析构策略引擎
  ~StrategyEngine();

  /// @brief 启动并订阅事件通道
  void Start();

  /// @brief 停止并取消订阅
  void Stop();

  /// @brief 注册策略（无品种路由，事件广播给全部策略）
  /// @param strategy 策略实例所有权
  void RegisterStrategy(std::unique_ptr<qtrade::strategy::IStrategy> strategy);

  /// @brief 注册策略并绑定唯一品种；重复绑定返回错误，不进入运行期
  /// @param strategy 策略实例所有权
  /// @param instruments 绑定品种列表
  /// @return ErrorCode::kSuccess 表示成功；品种冲突返回错误码
  ErrorCode RegisterStrategy(std::unique_ptr<qtrade::strategy::IStrategy> strategy,
                             const std::vector<std::string>& instruments);

  /// @brief 设置策略发单回调
  /// @param sender 发单函数
  void SetOrderSender(OrderSender sender);

 private:
  /// 事件通道引用
  event_bus::EventLanes& event_lanes_;
  /// 已注册策略列表
  std::vector<std::unique_ptr<qtrade::strategy::IStrategy>> strategies_;
  /// 品种 → 策略路由表；为空时退化为广播
  std::unordered_map<std::string, qtrade::strategy::IStrategy*> instrument_routes_;
  /// Market / Return 双线程回调共用此锁，串行进入策略
  std::mutex mutex_;
  /// 是否已 Start
  bool running_ = false;
  /// 发单回调
  OrderSender order_sender_;

  /// @brief 处理 Tick 事件并分发策略
  /// @param tick 行情 Tick
  void OnTickEvent(const qtrade_sdk::quote::MarketTick& tick);

  /// @brief 处理 Bar 事件并分发策略
  /// @param bar K 线
  void OnBarEvent(const qtrade_sdk::quote::Bar& bar);

  /// @brief 处理订单回报事件
  /// @param order 订单快照
  void OnOrderEvent(const qtrade_sdk::trader::Order& order);

  /// @brief 处理成交回报事件
  /// @param trade 成交快照
  void OnTradeEvent(const qtrade_sdk::trader::Trade& trade);
};

}  // namespace qtrade::engine::strategy

#endif  // QTRADE_TRADING_ENGINE_STRATEGY_ENGINE_HPP_
