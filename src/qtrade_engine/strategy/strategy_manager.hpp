/// @file      strategy_manager.hpp
/// @brief     策略管理器：装配插件实例、生命周期与事件分发绑定
/// @details   AddStrategyFromPlugin 按 .so 路径加载并注册；Start/Stop 对齐引擎生命周期。
///            不支持运行中按配置增删策略；配置变更须 Stop 后重新登记。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_

#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/strategy/strategy_event_dispatcher.hpp"
#include "qtrade/engine/strategy/strategy_plugin_loader.hpp"

#include <qtrade/error_code/error_codes.hpp>
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
using qtrade::strategy::StrategyConfig;

/// @brief 策略装载、启停与事件分发编排
class StrategyManager {
 public:
  /// @brief 构造策略管理器并绑定事件通道
  /// @param event_lanes Lane-Q / Lane-T 事件通道
  explicit StrategyManager(event_bus::EventLanes& event_lanes);

  /// @brief 析构：Stop 并卸载插件
  ~StrategyManager();

  StrategyManager(const StrategyManager&) = delete;
  StrategyManager& operator=(const StrategyManager&) = delete;

  /// @brief 按 .so 路径加载插件、创建并注册单个已启用策略
  /// @param config 策略实例配置；strategy_name 须与插件 ABI 名一致
  /// @param plugin_so_path 策略 .so 完整路径
  /// @param order_sender 发单回调（通常绑定 GetOrderPipeline().SubmitBatch，并做 READY 门禁）
  /// @return 成功返回 kSuccess；disabled 跳过亦返回成功；已 Start / 加载或注册失败时返回对应错误码
  ErrorCode AddStrategyFromPlugin(const StrategyConfig& config,
                                  const std::string& plugin_so_path,
                                  OrderSender order_sender);

  /// @brief Start 全部已注册策略
  /// @return 成功返回 kSuccess；已处于运行态返回 kAlreadyStarted
  ErrorCode Start();

  /// @brief Stop 全部策略、清空注册表并卸载插件
  /// @warning 须重新登记策略后才能再 Start
  void Stop();

  /// @brief 注册已 Init 完成的策略实例（单测注入；生产路径由 AddStrategyFromPlugin 内部调用）
  /// @param strategy_id 策略实例标识，不可为空
  /// @param strategy 策略实例所有权；不可为空
  /// @param instruments 本策略独占路由的合约列表；空表示仅参与广播
  /// @return 成功返回 kSuccess；参数非法 / 已 Start / ID 或合约冲突时返回错误码
  /// @warning 仅允许在未 Start 时调用
  ErrorCode RegisterStrategy(const std::string& strategy_id,
                             StrategyPtr strategy,
                             const std::vector<std::string>& instruments = {});

 private:
  /// @brief 已注册策略条目
  struct StrategyEntry {
    /// 策略实例（自定义删除器，与插件 ABI 对齐）
    StrategyPtr strategy{nullptr, [](IStrategy*) {}};
    /// 本策略独占路由的合约列表
    std::vector<std::string> instruments;
  };

  /// @brief 根据 strategies_ 构建广播用裸指针列表
  /// @return 当前全部策略裸指针
  [[nodiscard]] std::vector<IStrategy*> BuildStrategyListLocked() const;

  /// @brief 将当前路由快照推送给 dispatcher（若已创建）
  void PushRoutingToDispatcherLocked();

  /// 保护注册表与路由表
  mutable std::mutex mutex_;
  /// 是否已 Start
  std::atomic<bool> running_ = false;
  /// 事件通道（Lane-Q / Lane-T）
  event_bus::EventLanes& event_lanes_;
  /// 策略插件加载器
  StrategyPluginLoader plugin_loader_;
  /// 事件分发器；Init 后创建，Stop 时先于策略销毁
  std::unique_ptr<StrategyEventDispatcher> dispatcher_;
  /// strategy_id → 策略条目
  std::unordered_map<std::string, StrategyEntry> strategies_;
  /// 合约 → 独占该合约的策略；无条目时事件广播
  std::unordered_map<std::string, IStrategy*> instrument_routes_;
};

}  // namespace qtrade::engine::strategy

#endif  // QTRADE_TRADING_ENGINE_STRATEGY_MANAGER_HPP_
