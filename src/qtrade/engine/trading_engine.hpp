/// @file      trading_engine.hpp
/// @brief     交易引擎本体（Init / Start / Stop 与运行时编排）
/// @details   不负责进程入口；进程阶段由 apps/qtrade_engine/main.cpp + engine_boot 完成。
///            Init 只编排子阶段；EngineLifecycle 仅由本类推进，不下沉到子模块。
///            交易核心模块与 Client 由 EngineModules 持有。
///            须 Init 后 Start，仅 READY 接受新单。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
#define QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
#include "qtrade/common/config/qtrade_engine_config.hpp"
#include "qtrade/engine/core/engine_lifecycle.hpp"
#include "qtrade/engine/trading_engine_struct.hpp"
#include "qtrade/engine/core/quote_health_monitor.hpp"
#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/strategy/strategy_engine.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/account_risk/v1/account_risk.pb.h>
#include <qtrade/proto/config/v1/config.pb.h>
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

/// @brief 交易引擎：单进程封闭运行，整合行情、策略、OMS、EMS 等模块
class TradingEngine {
 public:
  TradingEngine();
  ~TradingEngine();
  TradingEngine(const TradingEngine&) = delete;
  TradingEngine& operator=(const TradingEngine&) = delete;

  // ---------------------------------------------------------------------------
  // 生命周期：Init → Start → Stop，以及状态查询
  // ---------------------------------------------------------------------------

  /// @brief 初始化引擎（编排 Init 子阶段，须在 Start 之前调用）
  /// @details 子阶段：ApplyBootstrapConfig → InitSupportClients → InitEngineModules
  ///          → InitEventLanes → InitAdapters（实现见 cpp 内部）
  /// @param config 进程引导配置（config/account 地址、tenant 等）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 启动运行时（编排 Start 子阶段，须先 Init）
  /// @details 子阶段：StartAdapters → SyncBrokerSnapshot → StartEventLanes
  ///          → StartEngineModules → StartMarketData → AdvanceReadyGates
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Start();

  /// @brief 停止所有子模块与 client（排空后调用 Release）
  /// @return 已运行并完成停机返回 kSuccess；未运行返回 kSystemError
  ErrorCode Stop();

  /// @brief 引擎是否处于运行中
  /// @return true 表示已 Start
  [[nodiscard]] bool IsRunning() const;

  /// @brief 引擎是否已通过全部 READY 门禁
  /// @return 仅生命周期为 READY 时返回 true
  [[nodiscard]] bool IsReady() const;

  /// @brief 释放资源
  void Release();

  /// @brief 查询当前生命周期状态
  /// @return 当前生命周期状态
  [[nodiscard]] EngineLifecycleState LifecycleState() const;

  /// @brief 返回当前进程引导配置快照
  /// @return 配置只读引用
  [[nodiscard]] const qtrade::common::config::QtradeEngineConfig& GetConfig() const;

  // ---------------------------------------------------------------------------
  // 交易入口：发单 / 撤单
  // ---------------------------------------------------------------------------

  /// @brief 将策略请求送入 CMS → Risk → OMS → EMS 发单链
  /// @param request 策略下单请求
  /// @return 生命周期为 READY 且准入成功时返回 kSuccess；未 READY 返回 kNotInitialized
  ErrorCode SubmitOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 撤销指定订单
  /// @param order_id 全局订单 ID
  /// @return 成功进入 EMS 撤单队列返回 kSuccess；订单不存在返回 kNotFound；
  ///         OMS 拒绝或 EMS 入队失败返回对应错误码
  ErrorCode CancelOrder(const std::string& order_id);

  // ---------------------------------------------------------------------------
  // 适配器与行情：注入 / 查询 / 订阅
  // ---------------------------------------------------------------------------

  /// @brief 绑定行情适配器（仅未 Start 时可设置；测试注入用）
  /// @param quote_api 行情适配器所有权
  void SetQuoteApi(std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api);

  /// @brief 绑定交易适配器（仅未 Start 时可设置；测试注入用）
  /// @param trader_api 交易适配器所有权
  void SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api);

  /// @brief 返回当前行情适配器
  /// @return 行情适配器指针；未设置时返回 nullptr
  [[nodiscard]] qtrade_sdk::quote::QuoteApi* GetQuoteApi();

  /// @brief 返回当前交易适配器
  /// @return 交易适配器指针；未设置时返回 nullptr
  [[nodiscard]] qtrade_sdk::trader::TraderApi* GetTraderApi();

  /// @brief 订阅合约行情（须已 Start 且行情通道已连接）
  /// @param instruments 合约列表
  void SubscribeQuote(const std::vector<std::string>& instruments);

  /// @brief 取消订阅合约行情
  /// @param instruments 合约列表
  void UnsubscribeQuote(const std::vector<std::string>& instruments);

  /// @brief 查询行情是否健康
  /// @return 已收到有效行情且未超时时返回 true
  [[nodiscard]] bool IsQuoteHealthy() const;

  // ---------------------------------------------------------------------------
  // 模块访问器
  // ---------------------------------------------------------------------------

  /// @brief 获取事件通道门面（Lane-Q + Lane-T）
  /// @return 事件通道引用
  event_bus::EventLanes& GetEventLanes();

  /// @brief 获取策略引擎引用
  /// @return StrategyEngine 引用
  strategy::StrategyEngine& GetStrategyEngine();

  /// @brief 获取订单管理模块引用
  /// @return OrderManager 引用
  oms::OrderManager& GetOrderManager();

  /// @brief 获取账户管理模块引用
  /// @return AccountManager 引用
  account::AccountManager& GetAccountManager();

  /// @brief 获取持仓管理模块引用
  /// @return PositionManager 引用
  position::PositionManager& GetPositionManager();

  /// @brief 获取配置客户端引用
  /// @return ConfigClient 引用
  client::ConfigClient& GetConfigClient();

 private:
  // ---------------------------------------------------------------------------
  // Init 子阶段（由 Init() 按序调用；失败时 Release）
  // ---------------------------------------------------------------------------

  /// @brief 缓存引导配置、配置行情健康阈值 → kBootstrap
  ErrorCode ApplyBootstrapConfig(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 初始化支撑服务客户端（config / account / account_risk）
  ErrorCode InitSupportClients();

  /// @brief 初始化引擎内模块（内存 OMS、account-risk 接线等）→ kReplayed
  ErrorCode InitEngineModules();

  /// @brief 初始化事件通道（本阶段仅确认就绪；Start 时再启动 reactor）
  ErrorCode InitEventLanes();

  /// @brief 按 EngineConfig 装配并连接行情/交易适配器（config 未启用时可跳过）
  ErrorCode InitAdapters();

  /// @brief 初始化并连接 config_client（GetEngineConfig + SubscribeEngineConfig）
  ErrorCode InitConfigClient(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 按配置初始化账户硬风控客户端
  ErrorCode InitAccountRiskClient(const qtrade::common::config::QtradeEngineConfig& config);

  // ---------------------------------------------------------------------------
  // Start 子阶段（由 Start() 按序调用）
  // ---------------------------------------------------------------------------

  /// @brief 幂等确认行情/交易适配器已连接
  ErrorCode StartAdapters();

  /// @brief 拉取柜台快照并 Adopt 进 OMS/Account/Position
  ErrorCode SyncBrokerSnapshot();

  /// @brief 启动 Lane-Q/Lane-T 与行情健康监控
  ErrorCode StartEventLanes();

  /// @brief 启动策略引擎与 EMS（Start 阶段的交易模块；Init 的 EngineModules 仅为 OMS/接线）
  ErrorCode StartEngineModules();

  /// @brief 按已缓存合约列表订阅行情
  ErrorCode StartMarketData();

  /// @brief 推进 BrokerSynced/RiskSynced，并按行情门禁尝试 READY
  ErrorCode AdvanceReadyGates();

  // ---------------------------------------------------------------------------
  // 适配器接线 / 断开
  // ---------------------------------------------------------------------------

  /// @brief 注册行情 SDK 回调并接入 Lane-Q
  void WireQuoteCallbacks();

  /// @brief 注册交易 SDK 回调并接入 Lane-T
  void WireTraderCallbacks();

  /// @brief 断开并释放行情/交易适配器
  void DisconnectAdapters();

  // ---------------------------------------------------------------------------
  // 柜台对账
  // ---------------------------------------------------------------------------

  /// @brief 查询柜台快照并完成启动对账
  ErrorCode SynchronizeBrokerState(qtrade_sdk::trader::TraderApi* trader_api);

  // ---------------------------------------------------------------------------
  // 运行时回调（配置热更新 / 行情健康 → 生命周期）
  // ---------------------------------------------------------------------------

  /// @brief 完整引擎配置回调：应用 EngineConfig
  void OnEngineConfig(const qtrade::config::v1::EngineConfig& config);

  /// @brief 处理行情健康变化并更新 READY 门禁
  void OnMarketHealthChanged(bool healthy);

  /// @brief 尽力调用 account-risk ReleaseOrder（无本地 outbox）
  void ReleaseAccountRiskReservation(const std::string& order_id,
                                     qtrade::account_risk::v1::ReleaseOrderRequest::Reason reason);

  // ---------------------------------------------------------------------------
  // 成员：生命周期与配置
  // ---------------------------------------------------------------------------

  /// 是否已完成 Init
  std::atomic_bool initialized_ = false;
  /// 是否已 Start
  std::atomic_bool running_ = false;
  /// 引擎生命周期状态机（仅本类读写）
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

  // ---------------------------------------------------------------------------
  // 成员：事件通道 / 策略 / 行情健康 / 适配器
  // ---------------------------------------------------------------------------

  /// Lane-Q / Lane-T 事件通道
  event_bus::EventLanes event_lanes_;
  /// 策略引擎
  strategy::StrategyEngine strategy_engine_;
  /// 行情健康监控
  QuoteHealthMonitor quote_health_monitor_;
  /// 行情适配器
  std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api_;
  /// 交易适配器
  std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api_;

  // ---------------------------------------------------------------------------
  // 成员：交易核心模块 + 支撑 Client
  // ---------------------------------------------------------------------------

  /// 交易核心模块与 gRPC Client（公开成员，见 trading_engine_struct.hpp）
  EngineModules modules_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
