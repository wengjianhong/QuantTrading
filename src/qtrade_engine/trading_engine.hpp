/// @file      trading_engine.hpp
/// @brief     交易引擎本体（Init / Start / Stop 与运行时编排）
/// @details   不负责进程入口；进程阶段由 apps/qtrade_engine/main.cpp + engine_boot 完成。
///            Init 只编排子阶段；EngineLifecycle 仅由本类推进，不下沉到子模块。
///            组合根持有各 Manager；支撑能力经 I*Bridge 注入；跨模块协作只通过 XxxApi。
///            须 Init 后 Start，仅 READY 接受新单。不依赖 proto / gRPC client。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
#define QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_

#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/account_risk/account_risk_manager.hpp"
#include "qtrade/engine/cms/compliance_manager.hpp"
#include "qtrade/engine/core/engine_lifecycle.hpp"
#include "qtrade/engine/core/lane_event_handler.hpp"
#include "qtrade/engine/core/order_pipeline.hpp"
#include "qtrade/engine/core/quote_health_monitor.hpp"
#include "qtrade/engine/core/sdk_event_handler.hpp"
#include "qtrade/engine/ems/execution_manager.hpp"
#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/oms/order_manager.hpp"
#include "qtrade/engine/position/position_manager.hpp"
#include "qtrade/engine/strategy/strategy_manager.hpp"

#include <qtrade/engine/engine.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/quote/quote_api.hpp>
#include <qtrade/sdk/trader/trader_api.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace qtrade::engine {

/// @brief 交易引擎实现：单进程封闭运行，整合行情、策略、OMS、EMS 等模块
/// @details 对外稳定契约见 IEngine。子模块由本类装配，不对外暴露访问器。
class TradingEngine final : public IEngine {
 public:
  TradingEngine();
  ~TradingEngine() override;
  TradingEngine(const TradingEngine&) = delete;
  TradingEngine& operator=(const TradingEngine&) = delete;

  // ---------------------------------------------------------------------------
  // IEngine：生命周期与状态
  // ---------------------------------------------------------------------------

  ErrorCode Init(const EngineConfig& config) override;
  ErrorCode Start() override;
  ErrorCode Stop() override;

  [[nodiscard]] EngineState State() const override;
  [[nodiscard]] bool IsRunning() const override;

  // ---------------------------------------------------------------------------
  // IEngine：依赖注入
  // ---------------------------------------------------------------------------

  void SetAccountBridge(qtrade::account::IAccountBridge* bridge) override;
  void SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* bridge) override;
  void SetQuoteApi(std::unique_ptr<qtrade::sdk::quote::QuoteApi> quote_api) override;
  void SetTraderApi(std::unique_ptr<qtrade::sdk::trader::TraderApi> trader_api) override;

  // ---------------------------------------------------------------------------
  // IEngine：策略登记（须在 Start 前）
  // ---------------------------------------------------------------------------

  ErrorCode AddStrategy(const qtrade::strategy::StrategyConfig& config, const std::string& plugin_so_path) override;

 private:
  // ---------------------------------------------------------------------------
  // Init 子阶段（由 Init() 按序调用；失败时 Release）
  // ---------------------------------------------------------------------------

  /// @brief 应用引擎运行配置（身份 / 行情源）
  /// @param config 引擎配置
  /// @return 成功返回 kSuccess；身份非法时返回错误码
  ErrorCode ApplyEngineConfig(const EngineConfig& config);

  /// @brief 配置行情健康阈值
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode ConfigureQuoteHealth();

  /// @brief 初始化引擎内模块（内存 OMS、account-risk 接线等）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitEngineModules();

  /// @brief 初始化事件通道（本阶段仅确认就绪；Start 时再启动 reactor）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitEventLanes();

  /// @brief Init 阶段适配器可延后注入；本阶段仅幂等确认
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitAdapters();

  // ---------------------------------------------------------------------------
  // Start 子阶段（由 Start() 按序调用）
  // ---------------------------------------------------------------------------

  /// @brief 启动 Lane-Q/Lane-T 与行情健康监控
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartEventLanes();

  /// @brief 确保行情/交易适配器已装配并连接（内部幂等调用 InitAdapters）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartAdapters();

  /// @brief 查询柜台订单/成交/持仓/资金并对账合并到 OMS/Account/Position
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode ReconcileBrokerState();

  /// @brief 启动策略管理器与 EMS
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartEngineModules();

  /// @brief 订阅策略登记时收集的合约行情
  /// @param instruments 合约列表
  void SubscribeQuote(const std::vector<std::string>& instruments);

  /// @brief 按行情门禁尝试进入 READY（Initiated → Ready）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode AdvanceReadyGates();

  // ---------------------------------------------------------------------------
  // Stop 子阶段（由 Stop() 按序调用）
  // ---------------------------------------------------------------------------

  /// @brief 释放资源（Init 失败路径与析构使用）
  void Release();

  /// @brief 断开并释放行情/交易适配器
  void DisconnectAdapters();

  // ---------------------------------------------------------------------------
  // 运行时
  // ---------------------------------------------------------------------------

  /// @brief 处理行情健康变化并更新 READY 门禁
  /// @param healthy 行情是否健康
  void HandleMarketHealthChanged(bool healthy);

  /// @brief 构造带 READY 门禁的策略发单回调，并自动填入 strategy_id
  /// @param strategy_id 策略 ID
  /// @return 策略发单回调
  [[nodiscard]] qtrade::strategy::OrderSender MakeOrderSender(std::string strategy_id);

  /// @brief 策略发单入口：READY 门禁、补全 strategy_id 后交给流水线
  /// @param strategy_id 登记策略时绑定的策略 ID
  /// @param batch 策略订单批次
  /// @return 未 READY 返回 kNotInitialized，否则返回流水线结果
  ErrorCode SubmitStrategyBatch(const std::string& strategy_id, const qtrade::strategy::OrderBatch& batch);

  // ---------------------------------------------------------------------------
  // 成员：生命周期与配置
  // ---------------------------------------------------------------------------

  /// 是否已完成 Init
  std::atomic<bool> initialized_ = false;
  /// 是否已 Start
  std::atomic<bool> running_ = false;
  /// 本进程启动世代（Init 时取 Unix 秒，写入 order_id）
  std::uint64_t engine_epoch_ = 0;
  /// 引擎生命周期状态机（仅本类读写）
  EngineLifecycle lifecycle_;
  /// 已应用的运行配置快照（Init 注入）
  EngineConfig engine_config_;
  /// 保护业务配置快照
  std::mutex engine_config_mutex_;
  /// 当前已订阅行情合约集合
  std::unordered_set<std::string> subscribed_instruments_;

  // ---------------------------------------------------------------------------
  // 成员：事件通道 / 策略 / 行情健康
  // ---------------------------------------------------------------------------

  /// Lane-Q / Lane-T 事件通道
  event_bus::EventLanes event_lanes_;
  /// 策略管理器（内部持有插件加载器）
  strategy::StrategyManager strategy_manager_;
  /// 行情健康监控
  QuoteHealthMonitor quote_health_monitor_;
  /// SDK 回调入站（须在 event_lanes / quote_health 之后）
  SdkEventHandler sdk_event_handler_{running_, event_lanes_, quote_health_monitor_};

  // ---------------------------------------------------------------------------
  // 成员：交易核心内的子模块
  // ---------------------------------------------------------------------------

  /// 合规模块（按 strategy_id 管理）
  cms::ComplianceManager compliance_;
  /// 订单管理
  oms::OrderManager order_manager_;
  /// 执行管理
  ems::ExecutionManager execution_manager_;
  /// 账户资金
  account::AccountManager account_manager_;
  /// 持仓
  position::PositionManager position_manager_;
  /// 账户硬风控（须在 pipeline / handler 之前；唯一持有 IAccountRiskBridge）
  account_risk::AccountRiskManager account_risk_manager_;
  /// 发单流水线（须在 compliance/order/execution/account_risk 之后）
  OrderPipeline order_pipeline_{compliance_, order_manager_, execution_manager_, account_risk_manager_};
  /// Lane-T 引擎侧回报处理（须在 order/account/position/account_risk 之后；Start 时先于策略 Dispatcher 注册）
  LaneEventHandler lane_event_handler_{order_manager_, account_manager_, position_manager_, account_risk_manager_};

  // ---------------------------------------------------------------------------
  // 成员：适配器
  // ---------------------------------------------------------------------------

  /// 行情适配器
  std::unique_ptr<qtrade::sdk::quote::QuoteApi> quote_api_;
  /// 交易适配器
  std::unique_ptr<qtrade::sdk::trader::TraderApi> trader_api_;

  // ---------------------------------------------------------------------------
  // 成员：支撑服务桥接（非拥有；由进程入口持有并注入）
  // ---------------------------------------------------------------------------

  /// 账户桥接
  qtrade::account::IAccountBridge* account_bridge_ = nullptr;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
