/// @file      trading_engine.hpp
/// @brief     交易引擎本体（Init / Start / Stop 与运行时编排）
/// @details   不负责进程入口；进程阶段由 apps/qtrade_engine/main.cpp + engine_boot 完成。
///            本类内部再拆 Init/Start 子阶段（围栏、回放、控制面、适配器、柜台对账等）。
///            须 Init 后 Start，仅 READY 接受新单。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
#define QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
#include "qtrade/client/account_client/account_client.hpp"
#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/client/config_client/config_client.hpp"
#include "qtrade/client/log_client/log_client.hpp"
#include "qtrade/client/monitor_client/monitor_client.hpp"
#include "qtrade/common/config/qtrade_engine_config.hpp"
#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/cms/compliance_manager.hpp"
#include "qtrade/engine/ems/execution_manager.hpp"
#include "qtrade/engine/engine_fence.hpp"
#include "qtrade/engine/engine_lifecycle.hpp"
#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/normalizer/quote_normalizer.hpp"
#include "qtrade/engine/normalizer/trader_normalizer.hpp"
#include "qtrade/engine/oms/order_manager.hpp"
#include "qtrade/engine/order_pipeline.hpp"
#include "qtrade/engine/position/position_manager.hpp"
#include "qtrade/engine/risk/account_risk_release_worker.hpp"
#include "qtrade/engine/risk/risk_manager.hpp"
#include "qtrade/engine/strategy/strategy_engine.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.pb.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>

namespace qtrade::engine {

/// @brief 交易引擎：单进程封闭运行，整合行情、策略、OMS、EMS 等模块
class TradingEngine {
 public:
  /// @brief 构造交易引擎（未初始化，须 Init 后 Start）
  TradingEngine();

  /// @brief 析构并 Stop
  ~TradingEngine();

  TradingEngine(const TradingEngine&) = delete;
  TradingEngine& operator=(const TradingEngine&) = delete;

  /// @brief 初始化引擎（编排 Init 子阶段，须在 Start 之前调用）
  /// @details 子阶段：BootstrapLocalRuntime → AcquireInstanceFence → ReplayLocalOrderFacts
  ///          → InitBypassClients → InitControlPlaneClients → ConnectAdaptersIfConfigured
  /// @param config 进程引导配置（config/account 地址、tenant、log/monitor 等）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 使用已加载的 config_ 初始化（须先 ReloadFromJson 或手动设置 config）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init();

  /// @brief 从本地 JSON 文件加载/重载引擎配置（如 config/qtrade_engine.json）
  /// @param json_path 配置文件路径
  /// @return ErrorCode::kSuccess 表示成功；文件不存在返回 ErrorCode::kNotFound
  ErrorCode ReloadFromJson(const std::string& json_path);

  /// @brief 返回当前进程引导配置快照
  /// @return 配置只读引用
  [[nodiscard]] const qtrade::common::config::QtradeEngineConfig& GetConfig() const;

  /// @brief 停止所有子模块与 client（排空后逆序释放）
  /// @return ErrorCode::kSuccess 表示成功；未运行返回 ErrorCode::kSystemError
  ErrorCode Stop();

  /// @brief 启动运行时（编排 Start 子阶段，须先 Init）
  /// @details 子阶段：EnsureAdaptersConnected → ReconcileBrokerState → StartRuntimeModules
  ///          → AdvancePostStartLifecycle（BrokerSynced/RiskSynced → 行情门禁 → Ready）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Start();

  /// @brief 引擎是否处于运行中
  /// @return true 表示已 Start
  [[nodiscard]] bool IsRunning() const;

  /// @brief 引擎是否已通过全部 READY 门禁
  /// @return 仅生命周期为 READY 时返回 true
  [[nodiscard]] bool IsReady() const;

  /// @brief 查询当前生命周期状态
  /// @return 当前生命周期状态
  [[nodiscard]] EngineLifecycleState LifecycleState() const;

  /// @brief 获取事件通道门面（Lane-M + Lane-R）
  /// @return 事件通道引用
  event_bus::EventLanes& GetEventLanes();

  /// @brief 获取行情标准化模块引用
  /// @return QuoteNormalizer 引用
  normalizer::QuoteNormalizer& GetQuoteNormalizer();

  /// @brief 获取交易标准化模块引用
  /// @return TraderNormalizer 引用
  normalizer::TraderNormalizer& GetTraderNormalizer();

  /// @brief 获取策略引擎引用
  /// @return StrategyEngine 引用
  strategy::StrategyEngine& GetStrategyEngine();

  /// @brief 获取日志客户端引用
  /// @return LogClient 引用
  client::LogClient& GetLogClient();

  /// @brief 获取监控客户端引用
  /// @return MonitorClient 引用
  client::MonitorClient& GetMonitorClient();

  /// @brief 获取配置客户端引用
  /// @return ConfigClient 引用
  client::ConfigClient& GetConfigClient();

  /// @brief 将策略请求送入 CMS → Risk → OMS → EMS 发单链
  /// @param request 策略下单请求
  /// @return 生命周期为 READY 且准入成功时返回 kSuccess；未 READY 返回 kNotInitialized
  ErrorCode SubmitOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 撤销指定订单
  /// @param order_id 全局订单 ID
  /// @return 成功进入 EMS 撤单队列返回 kSuccess；订单不存在返回 kNotFound；
  ///         OMS 拒绝或 EMS 入队失败返回对应错误码
  ErrorCode CancelOrder(const std::string& order_id);

  /// @brief 获取订单管理模块引用
  /// @return OrderManager 引用
  oms::OrderManager& GetOrderManager();

  /// @brief 获取账户管理模块引用
  /// @return AccountManager 引用
  account::AccountManager& GetAccountManager();

  /// @brief 获取持仓管理模块引用
  /// @return PositionManager 引用
  position::PositionManager& GetPositionManager();

 private:
  // ---------------------------------------------------------------------------
  // Init 子阶段（由 Init() 按序调用；失败时 ReleaseInitResources）
  // ---------------------------------------------------------------------------

  /// @brief Init-1: 应用引导配置并配置行情健康阈值 → kBootstrap
  ErrorCode BootstrapLocalRuntime(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief Init-2: 获取单实例写入围栏 → kFenced
  ErrorCode AcquireInstanceFence();

  /// @brief Init-3: 初始化订单 journal 并回放本地事实 → kReplayed
  ErrorCode ReplayLocalOrderFacts();

  /// @brief Init-4: 初始化 D 段旁路 client（log / monitor）
  ErrorCode InitBypassClients();

  /// @brief Init-5: 初始化控制面 client（config / account-risk）
  ErrorCode InitControlPlaneClients();

  /// @brief Init-6: 若启用 config-service，则按快照连接行情/交易适配器
  ErrorCode ConnectAdaptersIfConfigured();

  /// @brief Init 失败时逆序释放已初始化资源
  void ReleaseInitResources();

  // ---------------------------------------------------------------------------
  // Start 子阶段（由 Start() 按序调用）
  // ---------------------------------------------------------------------------

  /// @brief Start-1: 确保行情/交易适配器已连接（幂等）
  ErrorCode EnsureAdaptersConnected();

  /// @brief Start-2: 柜台订单/成交/资金/持仓对账
  ErrorCode ReconcileBrokerState();

  /// @brief Start-3: 启动 risk outbox、事件通道、标准化、策略与 EMS，并订阅合约
  ErrorCode StartRuntimeModules();

  /// @brief Start-4: 推进 BrokerSynced/RiskSynced，并按行情门禁尝试 READY
  ErrorCode AdvancePostStartLifecycle();

  // ---------------------------------------------------------------------------
  // 控制面 / 适配器辅助
  // ---------------------------------------------------------------------------

  /// @brief 初始化并连接 config_client（FetchSnapshot + StartWatch）
  ErrorCode InitConfigClient(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 按配置初始化账户硬风控客户端
  ErrorCode InitAccountRiskClient(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 按 config-service 下发的 EngineConfig 装配并连接行情/交易适配器
  ErrorCode InitAdapters();

  /// @brief 查询柜台快照并完成启动对账
  ErrorCode SynchronizeBrokerState(qtrade_sdk::trader::TraderApi* trader_api);

  /// @brief 完整引擎配置回调：应用 EngineConfig 并旁路日志
  void OnEngineConfig(const qtrade::config::v1::EngineConfig& config);

  /// @brief 处理行情健康变化并更新 READY 门禁
  void OnMarketHealthChanged(bool healthy);

  /// 是否已完成 Init
  std::atomic_bool initialized_ = false;
  /// 是否已 Start
  std::atomic_bool running_ = false;
  /// 单实例写入围栏
  EngineFence engine_fence_;
  /// 引擎生命周期状态机
  EngineLifecycle lifecycle_;
  /// 进程引导配置（qtrade_engine.json）
  qtrade::common::config::QtradeEngineConfig config_;
  /// config-service 下发的业务配置
  qtrade::config::v1::EngineConfig runtime_config_;
  /// 保护业务配置快照
  mutable std::mutex runtime_config_mutex_;
  /// 已应用的配置快照版本
  std::uint64_t runtime_config_version_ = 0;
  /// 当前已订阅行情合约集合
  std::unordered_set<std::string> subscribed_instruments_;
  /// Lane-M / Lane-R 事件通道
  event_bus::EventLanes event_lanes_;
  /// 策略引擎
  strategy::StrategyEngine strategy_engine_;
  /// 行情标准化
  normalizer::QuoteNormalizer quote_normalizer_;
  /// 交易标准化
  normalizer::TraderNormalizer trader_normalizer_;
  /// 合规模块
  cms::ComplianceManager compliance_;
  /// 执行管理模块
  ems::ExecutionManager execution_manager_;
  /// 订单管理模块
  oms::OrderManager order_manager_;
  /// 账户管理模块
  account::AccountManager account_manager_;
  /// 持仓管理模块
  position::PositionManager position_manager_;
  /// 风险管理模块
  risk::RiskManager risk_manager_;
  /// 发单准入流水线
  OrderPipeline order_pipeline_{compliance_, risk_manager_, order_manager_, execution_manager_};
  /// 控制面 gRPC 客户端
  client::ConfigClient config_client_;
  /// 账户凭证客户端
  client::AccountClient account_client_;
  /// E 段账户硬风控客户端
  client::AccountRiskClient account_risk_client_;
  /// E 段 Release 可靠 outbox
  risk::AccountRiskReleaseWorker account_risk_release_worker_;
  /// D 段日志旁路客户端
  client::LogClient log_client_;
  /// D 段监控旁路客户端
  client::MonitorClient monitor_client_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
