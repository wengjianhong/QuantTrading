/// @file      strategy_manager.hpp
/// @brief     策略管理器
/// @details   在引擎 Init 阶段注册已构造的策略实例；Start 启动、Stop 停止并清空。
///            不支持运行中按配置增删/启停策略；配置变更须 Stop 后重新 Init。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_
#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/strategy/strategy_plugin_loader.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::strategy {
using qtrade::strategy::IStrategy;

/// @brief 策略实例注册、行情/回报分发与发单回调桥接
class StrategyManager {
 public:
  /// @brief 构造策略管理器并绑定事件通道
  /// @param event_lanes Lane-Q / Lane-T 事件门面
  explicit StrategyManager(event_bus::EventLanes& event_lanes);

  /// @brief 析构策略管理器
  ~StrategyManager();

  /// @brief 初始化策略管理器
  ErrorCode Init();

  /// @brief 启动：订阅事件通道并 Start 全部已注册策略
  ErrorCode Start();

  /// @brief 停止全部策略并清空注册表
  /// @warning 须重新注册后才能再 Start
  void Stop();

  /// @brief 按稳定策略 ID 注册已构造并 Init 完成的实例
  /// @warning 仅允许在未 Start 时调用
  /// @param strategy_id 策略实例 ID
  /// @param strategy 策略实例所有权
  /// @param instruments 行情路由；同一品种只能归属一个策略
  /// @return 成功返回 kSuccess
  ErrorCode RegisterStrategy(const std::string& strategy_id,
                             StrategyPtr strategy,
                             const std::vector<std::string>& instruments = {});

 private:
  /// @brief 处理 Tick 事件并分发策略
  void OnTickEvent(const qtrade_sdk::quote::MarketTick& tick);

  /// @brief 处理 Bar 事件并分发策略
  void OnBarEvent(const qtrade_sdk::quote::Bar& bar);

  /// @brief 处理订单回报事件
  void OnOrderEvent(const qtrade_sdk::trader::Order& order);

  /// @brief 处理成交回报事件
  void OnTradeEvent(const qtrade_sdk::trader::Trade& trade);

 private:
  /// @brief 已注册策略条目
  struct StrategyEntry {
    /// 策略实例（插件路径使用 so 内 destroy）
    StrategyPtr strategy{nullptr, [](IStrategy*) {}};
    /// 当前行情路由
    std::vector<std::string> instruments;
  };

  /// 事件通道引用
  event_bus::EventLanes& event_lanes_;
  /// Market / Return 双线程回调共用此锁，串行进入策略
  mutable std::mutex mutex_;
  /// 策略 ID → 策略条目
  std::unordered_map<std::string, StrategyEntry> strategies_;
  /// 合约品种 → 策略路由表；为空时退化为全局广播
  std::unordered_map<std::string, IStrategy*> instrument_routes_;
  /// 是否运行中
  std::atomic_bool running_ = false;
};

}  // namespace qtrade::engine::strategy

#endif  // QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_
