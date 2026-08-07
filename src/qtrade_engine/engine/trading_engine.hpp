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

#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"
#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/cms/compliance_manager.hpp"
#include "qtrade/engine/core/engine_lifecycle.hpp"
#include "qtrade/engine/core/order_pipeline.hpp"
#include "qtrade/engine/core/quote_health_monitor.hpp"
#include "qtrade/engine/ems/execution_manager.hpp"
#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/oms/order_manager.hpp"
#include "qtrade/engine/position/position_manager.hpp"
#include "qtrade/engine/risk/risk_manager.hpp"
#include "qtrade/engine/strategy/strategy_manager.hpp"

#include <qtrade/engine/engine.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/quote/quote_api.hpp>
#include <qtrade_sdk/trader/trader_api.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace qtrade::engine {

/// @brief 交易引擎实现：单进程封闭运行，整合行情、策略、OMS、EMS 等模块
/// @details 对外稳定契约见 IEngine；本类额外提供内部模块访问器供 boot/测试使用。
class TradingEngine final : public IEngine {
 public:
  TradingEngine();
  ~TradingEngine() override;
  TradingEngine(const TradingEngine&) = delete;
  TradingEngine& operator=(const TradingEngine&) = delete;

  // ---------------------------------------------------------------------------
  // IEngine：依赖注入（须在 Init 前）
  // ---------------------------------------------------------------------------

  void SetConfigBridge(qtrade::config::IConfigBridge* bridge) override;
  void SetAccountBridge(qtrade::account::IAccountBridge* bridge) override;
  void SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* bridge) override;
  void SetQuoteApi(std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api) override;
  void SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api) override;

  // ---------------------------------------------------------------------------
  // IEngine：策略登记（须在 Start 前）
  // ---------------------------------------------------------------------------

  ErrorCode AddStrategy(const qtrade::strategy::StrategyConfig& config,
                        std::unique_ptr<qtrade::strategy::IStrategy> strategy) override;
  ErrorCode LoadStrategiesFromPlugins(const std::string& plugin_dir) override;

  // ---------------------------------------------------------------------------
  // IEngine：生命周期与状态
  // ---------------------------------------------------------------------------

  ErrorCode Init(const qtrade::common::config::QtradeEngineBootstrapConfig& config) override;
  ErrorCode Start() override;
  ErrorCode Stop() override;

  [[nodiscard]] EngineState State() const override;
  [[nodiscard]] bool IsRunning() const override;

  // ---------------------------------------------------------------------------
  // 实现侧扩展（非 IEngine；供 boot / 测试 / 内部编排）
  // ---------------------------------------------------------------------------

  /// @brief 释放资源（Init 失败路径与析构使用）
  void Release();

  /// @brief 是否已通过内部 READY 门禁（可接受新单）；仅实现/测试使用
  [[nodiscard]] bool IsReady() const;

  /// @brief 返回当前进程引导配置快照
  [[nodiscard]] const qtrade::common::config::QtradeEngineBootstrapConfig& GetBootstrapConfig() const;

  /// @brief 返回经配置桥接下发的引擎运行配置快照
  [[nodiscard]] qtrade::config::EngineConfig GetRuntimeConfig() const;

  /// @brief 返回当前行情适配器；未设置时返回 nullptr
  [[nodiscard]] qtrade_sdk::quote::QuoteApi* GetQuoteApi();

  /// @brief 返回当前交易适配器；未设置时返回 nullptr
  [[nodiscard]] qtrade_sdk::trader::TraderApi* GetTraderApi();

  /// @brief 订阅合约行情（须已 Start）；测试与内部配置热更新使用
  void SubscribeQuote(const std::vector<std::string>& instruments);

  /// @brief 取消订阅合约行情
  void UnsubscribeQuote(const std::vector<std::string>& instruments);

  /// @brief 查询行情是否健康
  [[nodiscard]] bool IsQuoteHealthy() const;

  // ---------------------------------------------------------------------------
  // 模块访问器
  // ---------------------------------------------------------------------------

  /// @brief 获取事件通道门面（Lane-Q + Lane-T）
  /// @return 事件通道引用
  event_bus::EventLanes& GetEventLanes();

  /// @brief 获取策略管理器引用
  /// @return StrategyManager 引用
  strategy::StrategyManager& GetStrategyManager();

  /// @brief 获取 OMS 模块间稳定接口
  /// @return OrderApi 引用
  oms::OrderApi& GetOrderApi();

  /// @brief 获取账户管理模块引用
  /// @return AccountManager 引用
  account::AccountManager& GetAccountManager();

  /// @brief 获取持仓管理模块引用
  /// @return PositionManager 引用
  position::PositionManager& GetPositionManager();

  /// @brief 获取发单流水线
  /// @return OrderPipeline 引用
  OrderPipeline& GetOrderPipeline();

 private:
  // ---------------------------------------------------------------------------
  // Init 子阶段（由 Init() 按序调用；失败时 Release）
  // ---------------------------------------------------------------------------

  /// @brief 缓存引导配置、配置行情健康阈值
  /// @param config 进程引导配置
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode ApplyBootstrapConfig(const qtrade::common::config::QtradeEngineBootstrapConfig& config);

  /// @brief 校验已注入的支撑桥接（config / account / account_risk）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitSupportBridges();

  /// @brief 初始化引擎内模块（内存 OMS、account-risk 接线等）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitEngineModules();

  /// @brief 初始化事件通道（本阶段仅确认就绪；Start 时再启动 reactor）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitEventLanes();

  /// @brief 按 EngineConfig 装配并连接行情/交易适配器（幂等；config 未启用时可跳过）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitAdapters();

  /// @brief 经配置桥接拉取并应用引擎运行配置
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode FetchRuntimeConfig();

  // ---------------------------------------------------------------------------
  // Start 子阶段（由 Start() 按序调用）
  // ---------------------------------------------------------------------------

  /// @brief 确保行情/交易适配器已装配并连接（内部幂等调用 InitAdapters）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartAdapters();

  /// @brief 拉取柜台快照并 Adopt 进 OMS/Account/Position
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode SyncBrokerSnapshot();

  /// @brief 启动 Lane-Q/Lane-T 与行情健康监控
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartEventLanes();

  /// @brief 启动策略管理器与 EMS
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartEngineModules();

  /// @brief 按已缓存合约列表订阅行情
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartMarketData();

  /// @brief 按行情门禁尝试进入 READY（Initiated → Ready）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode AdvanceReadyGates();

  // ---------------------------------------------------------------------------
  // 适配器接线 / 断开
  // ---------------------------------------------------------------------------

  /// @brief 注册行情 SDK 回调并接入 Lane-Q
  void WireQuoteCallbacks();

  /// @brief 注册交易 SDK 回调并接入 Lane-T
  void WireTraderCallbacks();

  /// @brief 注册 Lane-T 引擎级回报处理（OMS/账户/持仓/account-risk；Start 前调用）
  void WireTraderEventHandlers();

  /// @brief 断开并释放行情/交易适配器
  void DisconnectAdapters();

  // ---------------------------------------------------------------------------
  // 柜台对账
  // ---------------------------------------------------------------------------

  /// @brief 查询柜台快照并完成启动对账
  /// @param trader_api 交易适配器指针
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode SynchronizeBrokerState(qtrade_sdk::trader::TraderApi* trader_api);

  // ---------------------------------------------------------------------------
  // 运行时回调（配置应用 / 行情健康 → 生命周期）
  // ---------------------------------------------------------------------------

  /// @brief 完整引擎配置回调：应用 EngineConfig
  /// @param config 引擎配置
  void OnEngineConfig(const qtrade::config::EngineConfig& config);

  /// @brief 处理行情健康变化并更新 READY 门禁
  /// @param healthy 行情是否健康
  void OnMarketHealthChanged(bool healthy);

  /// @brief 处理 Lane-T 订单回报：更新 OMS/账户并在终态释放 account-risk 预占
  /// @param order 柜台订单回报
  void OnTraderOrderReport(const qtrade_sdk::trader::Order& order);

  /// @brief 处理 Lane-T 成交回报：更新 OMS/账户/持仓并在全成后释放 account-risk 预占
  /// @param trade 柜台成交回报
  void OnTraderTradeReport(const qtrade_sdk::trader::Trade& trade);

  /// @brief 尽力调用 account-risk ReleaseOrder（无本地 outbox）
  /// @param order_id 委托 ID
  /// @param reason 释放原因
  void ReleaseAccountRiskReservation(const std::string& order_id, qtrade::account_risk::ReleaseReason reason);

  /// @brief 构造带 READY 门禁的策略发单回调
  [[nodiscard]] qtrade::strategy::OrderSender MakeOrderSender();

  // ---------------------------------------------------------------------------
  // 成员：生命周期与配置
  // ---------------------------------------------------------------------------

  /// 是否已完成 Init
  std::atomic<bool> initialized_ = false;
  /// 是否已 Start
  std::atomic<bool> running_ = false;
  /// 本进程启动世代（Init 时取 Unix 秒，写入 order_id）
  std::uint64_t engine_epoch_ = 0;
  /// 已应用的配置快照版本
  std::uint64_t runtime_config_version_ = 0;
  /// 引擎生命周期状态机（仅本类读写）
  EngineLifecycle lifecycle_;
  /// 进程引导配置（qtrade_engine.json）
  qtrade::common::config::QtradeEngineBootstrapConfig bootstrap_config_;
  /// 经配置桥接下发的业务配置
  qtrade::config::EngineConfig runtime_config_;
  /// 保护业务配置快照
  mutable std::mutex runtime_config_mutex_;
  /// 当前已订阅行情合约集合
  std::unordered_set<std::string> subscribed_instruments_;

  // ---------------------------------------------------------------------------
  // 成员：事件通道 / 策略 / 行情健康 / 适配器
  // ---------------------------------------------------------------------------

  /// Lane-Q / Lane-T 事件通道
  event_bus::EventLanes event_lanes_;
  /// 策略管理器（内部持有插件加载器）
  strategy::StrategyManager strategy_manager_;
  /// 行情健康监控
  QuoteHealthMonitor quote_health_monitor_;
  /// 行情适配器
  std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api_;
  /// 交易适配器
  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api_;

  // ---------------------------------------------------------------------------
  // 成员：交易核心内的子模块
  // ---------------------------------------------------------------------------

  /// 合规模块
  cms::ComplianceManager compliance_;
  /// 实例风控
  risk::RiskManager risk_manager_;
  /// 订单管理
  oms::OrderManager order_manager_;
  /// 执行管理
  ems::ExecutionManager execution_manager_;
  /// 账户资金
  account::AccountManager account_manager_;
  /// 持仓
  position::PositionManager position_manager_;
  /// 发单流水线（须在 compliance/risk/order/execution 之后）
  OrderPipeline order_pipeline_{compliance_, risk_manager_, order_manager_, execution_manager_};

  // ---------------------------------------------------------------------------
  // 成员：支撑服务桥接（非拥有；由进程入口持有并注入）
  // ---------------------------------------------------------------------------

  /// 配置桥接
  qtrade::config::IConfigBridge* config_bridge_ = nullptr;
  /// 账户桥接
  qtrade::account::IAccountBridge* account_bridge_ = nullptr;
  /// 账户硬风控桥接
  qtrade::account_risk::IAccountRiskBridge* account_risk_bridge_ = nullptr;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
