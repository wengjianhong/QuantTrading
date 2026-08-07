/// @file      trading_engine.cpp
/// @brief     交易引擎实现
/// @details   实现顺序与 trading_engine.hpp 一致：构造析构 → 生命周期 → 交易入口 →
///            适配器/行情 → 模块访问器 → Init 子阶段 → Start 子阶段 → 接线/对账/回调。
///            EngineLifecycle 仅在本文件推进，不下沉到子模块。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/trading_engine.hpp"

#include "qtrade/common/system/time.hpp"
#include "qtrade/engine/utils/adapter_payload_validation.hpp"
#include "qtrade/error_code/error_codes.hpp"

#include <spdlog/spdlog.h>

#include <ctime>
#include <unordered_set>
#include <utility>
#include <vector>

namespace qtrade::engine {

using qtrade::engine::utils::IsValidBar;
using qtrade::engine::utils::IsValidOrder;
using qtrade::engine::utils::IsValidTick;
using qtrade::engine::utils::IsValidTrade;

// =============================================================================
// 构造 / 析构
// =============================================================================

TradingEngine::TradingEngine() : strategy_manager_(event_lanes_) {}

TradingEngine::~TradingEngine() {
  Stop();
}

void TradingEngine::SetConfigBridge(qtrade::config::IConfigBridge* bridge) {
  config_bridge_ = bridge;
}

void TradingEngine::SetAccountBridge(qtrade::account::IAccountBridge* bridge) {
  account_bridge_ = bridge;
}

void TradingEngine::SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* bridge) {
  account_risk_bridge_ = bridge;
}

qtrade::strategy::OrderSender TradingEngine::MakeOrderSender() {
  return [this](const qtrade::strategy::OrderBatch& batch) {
    if (!IsReady()) {
      return ErrorCode::kNotInitialized;
    }
    return order_pipeline_.SubmitBatch(batch);
  };
}

ErrorCode TradingEngine::AddStrategy(const qtrade::strategy::StrategyConfig& config,
                                     std::unique_ptr<qtrade::strategy::IStrategy> strategy) {
  if (!initialized_.load(std::memory_order_acquire)) {
    spdlog::error("AddStrategy requires Init first");
    return ErrorCode::kNotInitialized;
  }
  if (running_.load(std::memory_order_acquire)) {
    return ErrorCode::kAlreadyStarted;
  }
  if (!strategy || config.strategy_id.empty()) {
    return ErrorCode::kInvalidArgument;
  }
  if (!config.enabled) {
    spdlog::info("AddStrategy skip disabled strategy_id={}", config.strategy_id);
    return ErrorCode::kSuccess;
  }

  strategy->SetOrderSender(MakeOrderSender());
  if (strategy->Init(config) != ErrorCode::kSuccess) {
    spdlog::error("AddStrategy: strategy Init failed strategy_id={}", config.strategy_id);
    return ErrorCode::kInternalError;
  }

  qtrade::engine::strategy::StrategyPtr owned{strategy.release(), [](qtrade::strategy::IStrategy* raw) {
    delete raw;
  }};
  if (strategy_manager_.RegisterStrategy(config.strategy_id, std::move(owned), config.instruments) !=
      ErrorCode::kSuccess) {
    spdlog::error("AddStrategy: RegisterStrategy failed strategy_id={}", config.strategy_id);
    return ErrorCode::kSystemError;
  }
  spdlog::info("AddStrategy registered strategy_id={}", config.strategy_id);
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::LoadStrategiesFromPlugins(const std::string& plugin_dir) {
  if (!initialized_.load(std::memory_order_acquire)) {
    spdlog::error("LoadStrategiesFromPlugins requires Init first");
    return ErrorCode::kNotInitialized;
  }
  if (running_.load(std::memory_order_acquire)) {
    return ErrorCode::kAlreadyStarted;
  }
  if (plugin_dir.empty()) {
    spdlog::error("LoadStrategiesFromPlugins: plugin_dir is empty");
    return ErrorCode::kInvalidArgument;
  }

  const auto runtime_config = GetRuntimeConfig();
  const ErrorCode code =
    strategy_manager_.Init(plugin_dir, runtime_config.strategies, MakeOrderSender());
  if (code != ErrorCode::kSuccess) {
    spdlog::error("LoadStrategiesFromPlugins failed, code={}", static_cast<int>(code));
    return code;
  }
  return ErrorCode::kSuccess;
}

EngineState TradingEngine::State() const {
  return lifecycle_.State();
}

// =============================================================================
// 生命周期：Init → Start → Stop，以及状态查询
// =============================================================================

ErrorCode TradingEngine::Init(const qtrade::common::config::QtradeEngineBootstrapConfig& config) {
  if (initialized_) {
    return ErrorCode::kSuccess;
  }

  // 1. 初始化成员变量
  // 本进程启动世代（写入 order_id；须在 OMS Initialize 前赋值）
  engine_epoch_ = static_cast<std::uint64_t>(time(nullptr));

  // 2. 引导配置与行情健康阈值
  if (const ErrorCode code = ApplyBootstrapConfig(config); code != ErrorCode::kSuccess) {
    spdlog::error("ApplyBootstrapConfig failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 3. 校验已注入的支撑桥接（须在注入前由持有方就绪；引擎不 Start/Stop 桥接）
  if (const ErrorCode code = ValidateSupportBridges(); code != ErrorCode::kSuccess) {
    spdlog::error("ValidateSupportBridges failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 4. 拉取引擎运行配置 → runtime_config_（策略实例在 boot 按此加载）
  if (const ErrorCode code = FetchRuntimeConfig(); code != ErrorCode::kSuccess) {
    spdlog::error("FetchRuntimeConfig failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 5. 引擎内模块（内存 OMS 等）
  if (const ErrorCode code = InitEngineModules(); code != ErrorCode::kSuccess) {
    spdlog::error("InitEngineModules failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 6. 事件通道
  if (const ErrorCode code = InitEventLanes(); code != ErrorCode::kSuccess) {
    spdlog::error("InitEventLanes failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 7. 行情/交易适配器
  if (const ErrorCode code = InitAdapters(); code != ErrorCode::kSuccess) {
    spdlog::error("InitAdapters failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 8. 全部初始化完成 → Initiated
  if (lifecycle_.Transition(EngineState::kInitiated) != ErrorCode::kSuccess) {
    spdlog::error("EngineLifecycle Transition failed, reason={}", lifecycle_.Reason());
    lifecycle_.Transition(EngineState::kFailed, "INIT_STATE_TRANSITION_FAILED");
    Release();
    return ErrorCode::kSystemError;
  }

  initialized_ = true;
  spdlog::info("Init pipeline completed, state={}", static_cast<int>(lifecycle_.State()));
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::Start() {
  // 前置校验：须 Init 完成且生命周期处于 kInitiated
  if (!initialized_) {
    spdlog::error("Init TradingEngine must be called before Start TradingEngine.");
    return ErrorCode::kNotInitialized;
  }
  if (running_) {
    spdlog::error("TradingEngine is already running.");
    return ErrorCode::kAlreadyStarted;
  }
  if (lifecycle_.State() != EngineState::kInitiated) {
    spdlog::error("Invalid start state, current={}", lifecycle_.GetStateDescription(lifecycle_.State()));
    lifecycle_.Transition(EngineState::kFailed, "INVALID_START_STATE");
    return ErrorCode::kInvalidState;
  }

  // 1. 适配器就绪
  if (const ErrorCode code = StartAdapters(); code != ErrorCode::kSuccess) {
    spdlog::error("StartAdapters failed, code={}", static_cast<int>(code));
    return code;
  }

  // 2. 柜台快照 Adopt
  if (const ErrorCode code = SyncBrokerSnapshot(); code != ErrorCode::kSuccess) {
    spdlog::error("SyncBrokerSnapshot failed, code={}", static_cast<int>(code));
    return code;
  }

  // 3. 事件通道与行情健康监控
  if (const ErrorCode code = StartEventLanes(); code != ErrorCode::kSuccess) {
    spdlog::error("StartEventLanes failed, code={}", static_cast<int>(code));
    return code;
  }

  // 4. 策略 / EMS
  if (const ErrorCode code = StartEngineModules(); code != ErrorCode::kSuccess) {
    spdlog::error("StartEngineModules failed, code={}", static_cast<int>(code));
    return code;
  }

  // 5. 订阅行情
  if (const ErrorCode code = StartMarketData(); code != ErrorCode::kSuccess) {
    spdlog::error("StartMarketData failed, code={}", static_cast<int>(code));
    return code;
  }

  // 6. 生命周期门禁 → READY
  if (const ErrorCode code = AdvanceReadyGates(); code != ErrorCode::kSuccess) {
    spdlog::error("AdvanceReadyGates failed, code={}", static_cast<int>(code));
    return code;
  }

  spdlog::info("Start pipeline completed, state={}", lifecycle_.GetStateDescription(lifecycle_.State()));
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::Stop() {
  const bool was_running = running_.load(std::memory_order_acquire);
  if (was_running) {
    lifecycle_.Transition(EngineState::kDraining);
    running_.store(false, std::memory_order_release);
    spdlog::info("stopping components...");
  }

  Release();
  initialized_.store(false, std::memory_order_release);
  lifecycle_.Transition(EngineState::kStopped);

  if (!was_running) {
    return ErrorCode::kSystemError;
  }
  spdlog::info("stopped cleanly");
  return ErrorCode::kSuccess;
}

void TradingEngine::Release() {
  // 1. 释放引擎内模块（按依赖逆序释放）
  strategy_manager_.Stop();
  quote_health_monitor_.Stop();
  execution_manager_.Stop();
  DisconnectAdapters();
  event_lanes_.Stop();
  order_manager_.Shutdown();

  // 2. 释放适配器（桥接生命周期由进程入口持有）
  quote_api_.reset();
  trader_api_.reset();
}

bool TradingEngine::IsRunning() const {
  return running_.load(std::memory_order_acquire);
}

bool TradingEngine::IsReady() const {
  return lifecycle_.IsReady();
}

const qtrade::common::config::QtradeEngineBootstrapConfig& TradingEngine::GetBootstrapConfig() const {
  return bootstrap_config_;
}

qtrade::config::EngineConfig TradingEngine::GetRuntimeConfig() const {
  std::lock_guard lock(runtime_config_mutex_);
  return runtime_config_;
}

// =============================================================================
// 适配器与行情：注入 / 查询 / 订阅
// =============================================================================

void TradingEngine::SetQuoteApi(std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api) {
  if (running_) {
    return;
  }
  quote_api_ = std::move(quote_api);
  WireQuoteCallbacks();
}

void TradingEngine::SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api) {
  if (running_) {
    return;
  }
  trader_api_ = std::move(trader_api);
  WireTraderCallbacks();
}

qtrade_sdk::quote::QuoteApi* TradingEngine::GetQuoteApi() {
  return quote_api_.get();
}

qtrade_sdk::trader::TraderApi* TradingEngine::GetTraderApi() {
  return trader_api_.get();
}

void TradingEngine::SubscribeQuote(const std::vector<std::string>& instruments) {
  if (!running_ || quote_api_ == nullptr) {
    spdlog::warn("cannot subscribe quote: api not ready");
    return;
  }
  const auto rc = quote_api_->Subscribe({instruments});
  if (rc == ErrorCode::kSuccess) {
    spdlog::info("subscribed to {} instruments", instruments.size());
  } else {
    spdlog::error("quote subscription failed: {}", GetErrorCodeMessage(rc));
  }
}

void TradingEngine::UnsubscribeQuote(const std::vector<std::string>& instruments) {
  if (!running_ || quote_api_ == nullptr) {
    return;
  }
  quote_api_->Unsubscribe({instruments});
  spdlog::info("unsubscribed from {} instruments", instruments.size());
}

bool TradingEngine::IsQuoteHealthy() const {
  return quote_health_monitor_.IsHealthy();
}

// =============================================================================
// 模块访问器
// =============================================================================

event_bus::EventLanes& TradingEngine::GetEventLanes() {
  return event_lanes_;
}

strategy::StrategyManager& TradingEngine::GetStrategyManager() {
  return strategy_manager_;
}

oms::OrderApi& TradingEngine::GetOrderApi() {
  return order_manager_;
}

account::AccountManager& TradingEngine::GetAccountManager() {
  return account_manager_;
}

position::PositionManager& TradingEngine::GetPositionManager() {
  return position_manager_;
}

OrderPipeline& TradingEngine::GetOrderPipeline() {
  return order_pipeline_;
}

// =============================================================================
// Init 子阶段
// =============================================================================

ErrorCode TradingEngine::ApplyBootstrapConfig(const qtrade::common::config::QtradeEngineBootstrapConfig& config) {
  spdlog::info("ApplyBootstrapConfig");

  // 1. 缓存进程引导配置，供身份校验与 client 地址解析
  bootstrap_config_ = config;

  // 2. 配置行情陈旧阈值，供 READY 门禁与 OnMarketHealthChanged 使用
  QuoteHealthOptions quote_health_options;
  if (quote_health_monitor_.Configure(quote_health_options) != ErrorCode::kSuccess) {
    lifecycle_.Transition(EngineState::kFailed, "QUOTE_HEALTH_CONFIG_INVALID");
    return ErrorCode::kSystemError;
  }

  // 3. 行情健康变化驱动 READY 门禁（须在 Start 前注册）
  quote_health_monitor_.SetHealthChangedHandler([this](bool healthy) { OnMarketHealthChanged(healthy); });
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::ValidateSupportBridges() {
  spdlog::info("ValidateSupportBridges");

  if (bootstrap_config_.support_services.config_service.enabled && config_bridge_ == nullptr) {
    spdlog::error("config_service.enabled but config_bridge not set");
    lifecycle_.Transition(EngineState::kFailed, "CONFIG_BRIDGE_MISSING");
    return ErrorCode::kNotInitialized;
  }
  if (bootstrap_config_.support_services.account_service.enabled && account_bridge_ == nullptr) {
    spdlog::error("account_service.enabled but account_bridge not set");
    lifecycle_.Transition(EngineState::kFailed, "ACCOUNT_BRIDGE_MISSING");
    return ErrorCode::kNotInitialized;
  }
  if (bootstrap_config_.support_services.account_risk_service.enabled && account_risk_bridge_ == nullptr) {
    spdlog::error("account_risk_service.enabled but account_risk_bridge not set");
    lifecycle_.Transition(EngineState::kFailed, "ACCOUNT_RISK_BRIDGE_MISSING");
    return ErrorCode::kNotInitialized;
  }
  if (!bootstrap_config_.support_services.config_service.enabled) {
    spdlog::warn("config_service.enabled=false; runtime config via bridge optional");
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitEngineModules() {
  spdlog::info("InitEngineModules");

  // 1. OMS：仅内存状态机；冷启动不回放本地订单，Working 态由柜台快照对账重建
  oms::OrderManagerOptions order_options;
  order_options.engine_epoch = engine_epoch_;
  order_options.tenant_id = bootstrap_config_.config.identity.tenant_id;
  order_options.engine_id = bootstrap_config_.config.identity.engine_id;
  if (const auto rc = order_manager_.Initialize(order_options); rc != ErrorCode::kSuccess) {
    spdlog::error("order_manager init failed, code={}", static_cast<int>(rc));
    lifecycle_.Transition(EngineState::kFailed, "ORDER_MANAGER_INIT_FAILED");
    return rc;
  }

  // 2. 实例风控读 OMS 活动单与敞口（须在 OMS Initialize 之后）
  risk_manager_.SetStateProviders([this] { return order_manager_.GetActiveOrderCount(); },
                                  [this] { return order_manager_.GetOpenNotional(); });

  // 3. EMS 注入 OMS；发送失败释放预占所需的 account-risk（与 Pipeline 对称）
  execution_manager_.SetOrderApi(&order_manager_);
  if (account_risk_bridge_ != nullptr) {
    order_pipeline_.SetAccountRiskBridge(account_risk_bridge_);
    order_pipeline_.SetAccountRiskIdentity(bootstrap_config_.config.identity.tenant_id,
                                           bootstrap_config_.config.identity.account_id,
                                           bootstrap_config_.config.identity.engine_id);
    execution_manager_.SetAccountRiskBridge(account_risk_bridge_);
    execution_manager_.SetAccountRiskIdentity(bootstrap_config_.config.identity.tenant_id,
                                              bootstrap_config_.config.identity.account_id);
  }

  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitEventLanes() {
  spdlog::info("InitEventLanes");
  // Lane-Q / Lane-T 对象在构造时已就绪；订阅与 reactor 线程在 StartEventLanes 完成
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitAdapters() {
  // 适配器由调用方注入并完成 Connect；引擎不再按 execution_adapter 创建厂商实现
  if (quote_api_ != nullptr && trader_api_ != nullptr) {
    return ErrorCode::kSuccess;
  }
  if (!bootstrap_config_.support_services.config_service.enabled) {
    spdlog::info("skip adapters (config_service disabled; tests may inject stub)");
    return ErrorCode::kSuccess;
  }

  spdlog::error("quote/trader adapters not injected (SetQuoteApi/SetTraderApi required before Init/Start)");
  lifecycle_.Transition(EngineState::kFailed, "ADAPTER_NOT_INJECTED");
  return ErrorCode::kNotInitialized;
}

ErrorCode TradingEngine::FetchRuntimeConfig() {
  spdlog::info("FetchRuntimeConfig");
  if (config_bridge_ == nullptr) {
    spdlog::warn("config_bridge not set, skip FetchRuntimeConfig");
    return ErrorCode::kSuccess;
  }

  const auto result = config_bridge_->GetEngineConfig();
  if (result.error_code != ErrorCode::kSuccess || !result.data.has_value()) {
    lifecycle_.Transition(EngineState::kFailed, "GET_ENGINE_CONFIG_FAILED");
    return result.error_code;
  }
  OnEngineConfig(*result.data);
  return ErrorCode::kSuccess;
}

// =============================================================================
// Start 子阶段
// =============================================================================

ErrorCode TradingEngine::StartAdapters() {
  spdlog::info("StartAdapters");
  // 1. 幂等确保装配并连接（Init 阶段可能已完成）
  if (const auto result = InitAdapters(); result != ErrorCode::kSuccess) {
    if (lifecycle_.State() != EngineState::kFailed) {
      lifecycle_.Transition(EngineState::kFailed, "ADAPTER_NOT_READY");
    }
    return result;
  }

  // 2. 行情通道必须在线（有效行情仍由后续健康门禁决定是否 READY）
  if (quote_api_ == nullptr || !quote_api_->IsConnected()) {
    lifecycle_.Transition(EngineState::kFailed, "QUOTE_NOT_CONNECTED");
    return ErrorCode::kConnectionError;
  }

  // 3. 交易通道必须在线，否则无法对账与发单
  if (trader_api_ == nullptr || !trader_api_->IsConnected()) {
    lifecycle_.Transition(EngineState::kFailed, "TRADER_NOT_CONNECTED");
    return ErrorCode::kConnectionError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::SyncBrokerSnapshot() {
  spdlog::info("SyncBrokerSnapshot");
  // StartAdapters 已保证交易通道在线；拉柜台快照并对账，失败则拒绝 Start
  const auto sync_result = SynchronizeBrokerState(trader_api_.get());
  if (sync_result != ErrorCode::kSuccess) {
    lifecycle_.Transition(EngineState::kFailed, "BROKER_RECONCILIATION_FAILED");
    return sync_result;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::StartEventLanes() {
  spdlog::info("StartEventLanes");
  // Stop() 会清空 Lane-T 订阅；每次 Start 前重新注册引擎级回报处理
  WireTraderEventHandlers();
  event_lanes_.Start();
  quote_health_monitor_.Start();
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::StartEngineModules() {
  spdlog::info("StartEngineModules");
  // Start 阶段交易模块：策略消费与 EMS 出站（InitEngineModules 仅 OMS/接线）
  strategy_manager_.Start();
  execution_manager_.SetTraderApi(trader_api_.get());
  execution_manager_.Start();
  running_.store(true, std::memory_order_release);
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::StartMarketData() {
  spdlog::info("StartMarketData");
  std::vector<std::string> instruments;
  {
    std::lock_guard lock(runtime_config_mutex_);
    instruments.assign(subscribed_instruments_.begin(), subscribed_instruments_.end());
  }
  if (!instruments.empty()) {
    SubscribeQuote(instruments);
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::AdvanceReadyGates() {
  spdlog::info("AdvanceReadyGates");
  // 行情已健康则 Initiated → Ready；否则等 OnMarketHealthChanged
  if (quote_health_monitor_.IsHealthy()) {
    OnMarketHealthChanged(true);
  }
  return ErrorCode::kSuccess;
}

// =============================================================================
// 适配器接线 / 断开
// =============================================================================

void TradingEngine::WireQuoteCallbacks() {
  if (quote_api_ == nullptr) {
    return;
  }
  quote_api_->SetTickCallback([this](const qtrade_sdk::quote::MarketTick& tick) {
    if (!running_.load(std::memory_order_acquire)) {
      return;
    }
    if (!IsValidTick(tick)) {
      quote_health_monitor_.OnInvalidTick();
      spdlog::warn("rejected invalid tick: instrument={}", tick.instrument);
      return;
    }
    quote_health_monitor_.OnValidTick();
    event_lanes_.Quote().PublishTick(tick);
  });
  quote_api_->SetBarCallback([this](const qtrade_sdk::quote::Bar& bar) {
    if (!running_.load(std::memory_order_acquire) || !IsValidBar(bar)) {
      return;
    }
    event_lanes_.Quote().PublishBar(bar);
  });
}

void TradingEngine::WireTraderCallbacks() {
  if (trader_api_ == nullptr) {
    return;
  }
  trader_api_->SetOrderCallback([this](const qtrade_sdk::trader::Order& order) {
    if (!running_.load(std::memory_order_acquire) || !IsValidOrder(order)) {
      return;
    }
    event_lanes_.Trader().PublishOrder(order);
  });
  trader_api_->SetTradeCallback([this](const qtrade_sdk::trader::Trade& trade) {
    if (!running_.load(std::memory_order_acquire) || !IsValidTrade(trade)) {
      return;
    }
    event_lanes_.Trader().PublishTrade(trade);
  });
}

void TradingEngine::WireTraderEventHandlers() {
  // Trader Lane：订单/成交回报的唯一异步入口，串联 OMS、账户、持仓与 account-risk 释放
  event_lanes_.Trader().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) { OnTraderOrderReport(order); });
  event_lanes_.Trader().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) { OnTraderTradeReport(trade); });
}

void TradingEngine::DisconnectAdapters() {
  if (quote_api_ != nullptr && quote_api_->IsConnected()) {
    quote_api_->Disconnect();
  }
  if (trader_api_ != nullptr && trader_api_->IsConnected()) {
    trader_api_->Disconnect();
  }
}

// =============================================================================
// 柜台对账
// =============================================================================

ErrorCode TradingEngine::SynchronizeBrokerState(qtrade_sdk::trader::TraderApi* trader_api) {
  // 1. 查询柜台订单、成交、持仓与资金快照
  if (trader_api == nullptr || !trader_api->IsConnected()) {
    return ErrorCode::kConnectionError;
  }

  qtrade_sdk::trader::QueryOrdersResponse orders_response;
  qtrade_sdk::trader::QueryTradesResponse trades_response;
  qtrade_sdk::trader::QueryPositionResponse positions_response;
  qtrade_sdk::trader::QueryAssetResponse asset_response;
  if (trader_api->QueryOrders({}, orders_response) != ErrorCode::kSuccess ||
      trader_api->QueryTrades({}, trades_response) != ErrorCode::kSuccess ||
      trader_api->QueryPositions({}, positions_response) != ErrorCode::kSuccess) {
    return ErrorCode::kNotSupported;
  }
  qtrade_sdk::trader::QueryAssetRequest asset_request;
  asset_request.account_id = bootstrap_config_.config.identity.account_id;
  if (trader_api->QueryAsset(asset_request, asset_response) != ErrorCode::kSuccess) {
    return ErrorCode::kNotSupported;
  }

  // 2. 应用柜台订单快照（可冷启动采纳进内存 OMS，不补单）并校验待对账订单已清空
  for (const auto& report : orders_response.orders) {
    order_manager_.ReconcileBrokerOrder(report);
    const auto local = report.order_id.empty() ? order_manager_.GetOrderByClientId(report.client_order_id)
                                               : order_manager_.GetOrder(report.order_id);
    if (local.has_value()) {
      order_manager_.MarkReconciled(local->order_id);
      account_manager_.ApplyOrder(*local);
    }
  }
  for (const auto& report : trades_response.trades) {
    order_manager_.ApplyTradeReport(report);
    const auto local = report.order_id.empty() ? order_manager_.GetOrderByClientId(report.client_order_id)
                                               : order_manager_.GetOrder(report.order_id);
    if (local.has_value()) {
      order_manager_.MarkReconciled(local->order_id);
    }
  }
  position_manager_.ApplyPositionSnapshot(positions_response.positions);
  account_manager_.ApplyAssetSnapshot(asset_response.asset);
  return order_manager_.GetOrdersRequiringReconciliation().empty() ? ErrorCode::kSuccess : ErrorCode::kInternalError;
}

// =============================================================================
// 运行时回调（配置推送 / 行情健康 → 生命周期）
// =============================================================================

void TradingEngine::OnEngineConfig(const qtrade::config::EngineConfig& config) {
  if (config.version == 0) {
    spdlog::warn("invalid engine config version={}", config.version);
    lifecycle_.Transition(EngineState::kFrozen, "CONFIG_SNAPSHOT_INVALID");
    return;
  }

  if (config.engine_id != bootstrap_config_.config.identity.engine_id ||
      config.tenant_id != bootstrap_config_.config.identity.tenant_id ||
      config.account_id != bootstrap_config_.config.identity.account_id) {
    spdlog::error("config identity mismatch");
    lifecycle_.Transition(EngineState::kFrozen, "CONFIG_IDENTITY_MISMATCH");
    return;
  }
  const auto now_ms = qtrade::common::system::UnixMillisNow();
  if (config.valid_until_unix_ms > 0 && now_ms >= config.valid_until_unix_ms) {
    spdlog::error("rejected expired engine config version={}", config.version);
    lifecycle_.Transition(EngineState::kFrozen, "CONFIG_EXPIRED");
    return;
  }

  {
    std::lock_guard lock(runtime_config_mutex_);
    if (config.version <= runtime_config_version_) {
      spdlog::warn("ignored stale engine config version={}", config.version);
      return;
    }
    if (running_.load(std::memory_order_acquire) && runtime_config_version_ != 0) {
      lifecycle_.Transition(EngineState::kFrozen, "CONFIG_RESTART_REQUIRED");
      spdlog::error("engine config version changed while running (loaded={}, incoming={}); stop and re-init required",
                    runtime_config_version_,
                    config.version);
      return;
    }
  }

  risk::RiskLimits limits;
  limits.version = config.risk_budget.version != 0 ? config.risk_budget.version : config.version;
  limits.max_order_notional = config.risk_budget.max_notional;
  limits.max_total_notional = config.risk_budget.max_notional;
  limits.max_open_orders = config.risk_budget.max_open_orders;
  limits.safety_buffer = config.risk_budget.safety_buffer;
  if (risk_manager_.Configure(limits) != ErrorCode::kSuccess) {
    lifecycle_.Transition(EngineState::kFrozen, "RISK_CONFIG_INVALID");
    return;
  }

  if (!running_.load(std::memory_order_acquire)) {
    spdlog::info("cached {} strategy config(s); instances will be created by LoadStrategies",
                 config.strategies.size());
  }

  std::unordered_set<std::string> desired_instruments;
  for (const auto& strategy : config.strategies) {
    if (!strategy.enabled) {
      continue;
    }
    for (const auto& instrument : strategy.instruments) {
      if (!instrument.empty()) {
        desired_instruments.insert(instrument);
      }
    }
  }
  cms::ComplianceRules compliance_rules;
  compliance_rules.version = config.version;
  compliance_rules.allowed_instruments = desired_instruments;
  compliance_rules.max_notional = config.risk_budget.max_notional;
  if (compliance_.Configure(compliance_rules) != ErrorCode::kSuccess) {
    lifecycle_.Transition(EngineState::kFrozen, "COMPLIANCE_CONFIG_INVALID");
    return;
  }

  std::vector<std::string> to_subscribe;
  std::vector<std::string> to_unsubscribe;
  {
    std::lock_guard lock(runtime_config_mutex_);
    for (const auto& instrument : desired_instruments) {
      if (!subscribed_instruments_.contains(instrument)) {
        to_subscribe.push_back(instrument);
      }
    }
    for (const auto& instrument : subscribed_instruments_) {
      if (!desired_instruments.contains(instrument)) {
        to_unsubscribe.push_back(instrument);
      }
    }
    runtime_config_ = config;
    runtime_config_version_ = config.version;
    subscribed_instruments_ = std::move(desired_instruments);
  }

  if (running_.load(std::memory_order_acquire)) {
    if (!to_unsubscribe.empty()) {
      UnsubscribeQuote(to_unsubscribe);
    }
    if (!to_subscribe.empty()) {
      SubscribeQuote(to_subscribe);
    }
  }

  spdlog::info("config snapshot version={}, account={}, quote_source={}, strategies={}",
               config.version,
               bootstrap_config_.config.identity.account_id,
               config.quote_source,
               config.strategies.size());

  for (const auto& strategy : config.strategies) {
    spdlog::info("strategy {} enabled={}", strategy.strategy_id, strategy.enabled);
  }
}

void TradingEngine::OnTraderOrderReport(const qtrade_sdk::trader::Order& order) {
  order_manager_.ApplyOrderReport(order);
  const auto local_order = order.order_id.empty() ? order_manager_.GetOrderByClientId(order.client_order_id)
                                                  : order_manager_.GetOrder(order.order_id);
  if (local_order.has_value()) {
    account_manager_.ApplyOrder(*local_order);
  }

  // 拒单/撤单完成时释放 account-risk 预占（直接 gRPC，无本地 outbox）
  if (order.status != qtrade_sdk::trader::OrderStatusType::kRejected &&
      order.status != qtrade_sdk::trader::OrderStatusType::kCanceled) {
    return;
  }
  if (!local_order.has_value()) {
    return;
  }
  const auto reason = order.status == qtrade_sdk::trader::OrderStatusType::kCanceled
                        ? qtrade::account_risk::ReleaseReason::kCanceled
                        : qtrade::account_risk::ReleaseReason::kRejectedByVenue;
  ReleaseAccountRiskReservation(local_order->order_id, reason);
}

void TradingEngine::OnTraderTradeReport(const qtrade_sdk::trader::Trade& trade) {
  order_manager_.ApplyTradeReport(trade);
  account_manager_.ApplyTrade(trade);
  position_manager_.ApplyTrade(trade);

  // 全部成交后释放风控预占（SETTLED）
  const auto local_order = trade.order_id.empty() ? order_manager_.GetOrderByClientId(trade.client_order_id)
                                                  : order_manager_.GetOrder(trade.order_id);
  if (local_order.has_value() && local_order->status == qtrade_sdk::trader::OrderStatusType::kFilled) {
    ReleaseAccountRiskReservation(local_order->order_id, qtrade::account_risk::ReleaseReason::kSettled);
  }
}

void TradingEngine::OnMarketHealthChanged(bool healthy) {
  const auto state = lifecycle_.State();

  // 1. 行情不健康：Ready → Frozen（拒新单，仍处理回报）
  if (!healthy) {
    if (state == EngineState::kReady) {
      lifecycle_.Transition(EngineState::kFrozen, "MARKET_UNHEALTHY");
      spdlog::warn("engine frozen: market unhealthy");
    }
    return;
  }

  // 2. 行情恢复：Initiated → Ready，或从 MARKET_UNHEALTHY 冻结恢复
  if (state == EngineState::kInitiated) {
    if (lifecycle_.Transition(EngineState::kReady) == ErrorCode::kSuccess) {
      spdlog::info("trading engine READY");
    }
  } else if (state == EngineState::kFrozen && lifecycle_.Reason() == "MARKET_UNHEALTHY") {
    if (lifecycle_.Transition(EngineState::kReady) == ErrorCode::kSuccess) {
      spdlog::info("trading engine resumed after market recovery");
    }
  }
}

void TradingEngine::ReleaseAccountRiskReservation(const std::string& order_id,
                                                  qtrade::account_risk::ReleaseReason reason) {
  if (account_risk_bridge_ == nullptr || order_id.empty()) {
    return;
  }
  const auto result = account_risk_bridge_->ReleaseOrder(bootstrap_config_.config.identity.tenant_id,
                                                         bootstrap_config_.config.identity.account_id,
                                                         order_id,
                                                         reason,
                                                         0.0,
                                                         0.0);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("ReleaseOrder failed: order_id={}, code={}", order_id, static_cast<int>(result.error_code));
  }
}

std::unique_ptr<IEngine> CreateEngine() {
  return std::make_unique<TradingEngine>();
}

}  // namespace qtrade::engine
