/// @file      strategy_manager.hpp
/// @brief     策略管理器：装配插件实例、生命周期与事件分发绑定
/// @details   Init 加载插件并注册策略；Start/Stop 对齐引擎生命周期。
///            不支持运行中按配置增删策略；配置变更须 Stop 后重新 Init。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/strategy/strategy_event_dispatcher.hpp"
#include "qtrade/engine/strategy/strategy_plugin_loader.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.pb.h>
#include <qtrade/strategy/strategy.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::strategy {
using qtrade::strategy::IStrategy;
using qtrade::strategy::OrderSender;

/// @brief 策略装载、启停与事件分发编排
class StrategyManager {
 public:
  /// @brief 构造策略管理器并绑定事件通道
  explicit StrategyManager(event_bus::EventLanes& event_lanes);

  /// @brief 析构：Stop 并卸载插件
  ~StrategyManager();

  StrategyManager(const StrategyManager&) = delete;
  StrategyManager& operator=(const StrategyManager&) = delete;

  /// @brief 加载插件目录、按配置创建并注册已启用策略，订阅事件分发
  /// @param plugin_dir 策略 .so 目录
  /// @param strategies runtime_config.strategies
  /// @param order_sender 发单回调（通常绑定引擎 SubmitOrder）
  /// @return 成功返回 kSuccess
  ErrorCode Init(const std::string& plugin_dir,
                 const google::protobuf::RepeatedPtrField<qtrade::config::v1::StrategyConfig>& strategies,
                 OrderSender order_sender);

  /// @brief Start 全部已注册策略
  ErrorCode Start();

  /// @brief Stop 全部策略、清空注册表并卸载插件
  /// @warning 须重新 Init 后才能再 Start
  void Stop();

  /// @brief 注册已 Init 完成的策略实例（单测注入；生产路径由 Init 内部调用）
  /// @warning 仅允许在未 Start 时调用
  ErrorCode RegisterStrategy(const std::string& strategy_id,
                             StrategyPtr strategy,
                             const std::vector<std::string>& instruments = {});

 private:
  struct StrategyEntry {
    StrategyPtr strategy{nullptr, [](IStrategy*) {}};
    std::vector<std::string> instruments;
  };

  void RebuildStrategyListLocked();

  event_bus::EventLanes& event_lanes_;
  StrategyPluginLoader plugin_loader_;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, StrategyEntry> strategies_;
  std::unordered_map<std::string, IStrategy*> instrument_routes_;
  std::vector<IStrategy*> strategy_list_;
  std::atomic_bool running_{false};
  std::unique_ptr<StrategyEventDispatcher> dispatcher_;
};

}  // namespace qtrade::engine::strategy

#endif  // QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_
