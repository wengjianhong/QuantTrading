/// @file      trading_engine.cpp
/// @brief     交易引擎实现
/// @details   实现顺序与 trading_engine.hpp 声明一致。
///            EngineLifecycle 仅在本文件推进，不下沉到子模块。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/trading_engine.hpp"

#include "qtrade/common/utils/adapter_payload_validation.hpp"
#include "qtrade/error_code/error_codes.hpp"

#include <spdlog/spdlog.h>

#include <ctime>
#include <filesystem>
#include <unordered_set>
#include <utility>
#include <vector>

namespace qtrade::engine {

using qtrade::common::utils::IsValidBar;
using qtrade::common::utils::IsValidOrder;
using qtrade::common::utils::IsValidTick;
using qtrade::common::utils::IsValidTrade;

// =============================================================================
// 构造 / 析构
// =============================================================================

TradingEngine::TradingEngine() : strategy_manager_(event_lanes_) {}

TradingEngine::~TradingEngine() {
  Stop();
}

// =============================================================================
// IEngine：生命周期与状态
// =============================================================================

ErrorCode TradingEngine::Init(const EngineConfig& config) {
  if (initialized_) {
    return ErrorCode::kSuccess;
  }

  // 1. 本进程启动世代（写入 order_id；须在 OMS Initialize 前赋值）
  engine_epoch_ = static_cast<std::uint64_t>(time(nullptr));

  // 2. 行情健康阈值
  if (const ErrorCode code = ConfigureQuoteHealth(); code != ErrorCode::kSuccess) {
    spdlog::error("ConfigureQuoteHealth failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 3. 应用运行配置（身份 + 风控/合规）
  if (const ErrorCode code = ApplyEngineConfig(config); code != ErrorCode::kSuccess) {
    spdlog::error("ApplyEngineConfig failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 4. 引擎内模块（内存 OMS 等）
  if (const ErrorCode code = InitEngineModules(); code != ErrorCode::kSuccess) {
    spdlog::error("InitEngineModules failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 5. 事件通道
  if (const ErrorCode code = InitEventLanes(); code != ErrorCode::kSuccess) {
    spdlog::error("InitEventLanes failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 6. 适配器可在 Init 后、Start 前注入
  if (const ErrorCode code = InitAdapters(); code != ErrorCode::kSuccess) {
    spdlog::error("InitAdapters failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 7. 全部初始化完成 → Initiated
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
  if (running_.load(std::memory_order_acquire)) {
    spdlog::error("TradingEngine is already running.");
    return ErrorCode::kAlreadyStarted;
  }
  if (lifecycle_.State() != EngineState::kInitiated) {
    spdlog::error("Invalid start state, current={}", lifecycle_.GetStateDescription(lifecycle_.State()));
    lifecycle_.Transition(EngineState::kFailed, "INVALID_START_STATE");
    return ErrorCode::kInvalidState;
  }

  // 1. 事件通道与行情健康监控
  if (const ErrorCode code = StartEventLanes(); code != ErrorCode::kSuccess) {
    spdlog::error("StartEventLanes failed, code={}", static_cast<int>(code));
    return code;
  }

  // 2. 适配器就绪
  if (const ErrorCode code = StartAdapters(); code != ErrorCode::kSuccess) {
    spdlog::error("StartAdapters failed, code={}", static_cast<int>(code));
    return code;
  }

  // 3. 柜台对账
  if (const ErrorCode code = ReconcileBrokerState(); code != ErrorCode::kSuccess) {
    spdlog::error("ReconcileBrokerState failed, code={}", static_cast<int>(code));
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

EngineState TradingEngine::State() const {
  return lifecycle_.State();
}

bool TradingEngine::IsRunning() const {
  return running_.load(std::memory_order_acquire);
}

// =============================================================================
// IEngine：依赖注入
// =============================================================================

void TradingEngine::SetAccountBridge(qtrade::account::IAccountBridge* bridge) {
  account_bridge_ = bridge;
}

void TradingEngine::SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* bridge) {
  account_risk_bridge_ = bridge;
}

void TradingEngine::SetQuoteApi(std::unique_ptr<qtrade::sdk::quote::QuoteApi> quote_api) {
  if (running_.load(std::memory_order_acquire)) {
    return;
  }
  quote_api_ = std::move(quote_api);
  WireQuoteCallbacks();
}

void TradingEngine::SetTraderApi(std::unique_ptr<qtrade::sdk::trader::TraderApi> trader_api) {
  if (running_.load(std::memory_order_acquire)) {
    return;
  }
  trader_api_ = std::move(trader_api);
  WireTraderCallbacks();
}

// =============================================================================
// IEngine：策略登记（须在 Start 前）
// =============================================================================

ErrorCode TradingEngine::AddStrategy(const qtrade::strategy::StrategyConfig& config,
                                     const std::string& plugin_so_path) {
  if (!initialized_.load(std::memory_order_acquire)) {
    spdlog::error("AddStrategy requires Init first");
    return ErrorCode::kNotInitialized;
  }
  if (running_.load(std::memory_order_acquire)) {
    return ErrorCode::kAlreadyStarted;
  }
  if (config.strategy_id.empty()) {
    return ErrorCode::kInvalidArgument;
  }
  if (plugin_so_path.empty()) {
    spdlog::error("AddStrategy: plugin_so_path is empty strategy_id={}", config.strategy_id);
    return ErrorCode::kInvalidArgument;
  }
  {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(plugin_so_path, ec)) {
      spdlog::error("AddStrategy: plugin so not found path={} strategy_id={} ({})",
                    plugin_so_path,
                    config.strategy_id,
                    ec.message());
      return ErrorCode::kNotSuchFileOrDirectory;
    }
  }

  const ErrorCode code =
    strategy_manager_.AddStrategyFromPlugin(config, plugin_so_path, MakeOrderSender(config.strategy_id));
  if (code != ErrorCode::kSuccess) {
    return code;
  }
  if (!config.enabled) {
    return ErrorCode::kSuccess;
  }

  {
    std::lock_guard lock(engine_config_mutex_);
    for (const auto& instrument : config.instruments) {
      if (!instrument.empty()) {
        subscribed_instruments_.insert(instrument);
      }
    }
  }

  auto risk = config.risk;
  if (risk.max_volume <= 0 && config.args.order_volume > 0) {
    risk.max_volume = config.args.order_volume;
  }

  if (const auto rc = compliance_.UpsertStrategyRules(config.strategy_id, risk); rc != ErrorCode::kSuccess) {
    lifecycle_.Transition(EngineState::kFailed, "STRATEGY_COMPLIANCE_INVALID");
    return rc;
  }
  return ErrorCode::kSuccess;
}

// =============================================================================
// 实现侧扩展（非 IEngine）
// =============================================================================

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

bool TradingEngine::IsReady() const {
  return lifecycle_.IsReady();
}

bool TradingEngine::IsQuoteHealthy() const {
  return quote_health_monitor_.IsHealthy();
}

void TradingEngine::SubscribeQuote(const std::vector<std::string>& instruments) {
  if (!running_.load(std::memory_order_acquire) || quote_api_ == nullptr) {
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
  if (!running_.load(std::memory_order_acquire) || quote_api_ == nullptr) {
    return;
  }
  quote_api_->Unsubscribe({instruments});
  spdlog::info("unsubscribed from {} instruments", instruments.size());
}

// =============================================================================
// 模块访问器
// =============================================================================

EngineConfig TradingEngine::GetRuntimeConfig() const {
  std::lock_guard lock(engine_config_mutex_);
  return engine_config_;
}

// =============================================================================
// Init 子阶段
// =============================================================================

ErrorCode TradingEngine::ConfigureQuoteHealth() {
  spdlog::info("ConfigureQuoteHealth");
  QuoteHealthOptions quote_health_options;
  if (quote_health_monitor_.Configure(quote_health_options) != ErrorCode::kSuccess) {
    lifecycle_.Transition(EngineState::kFailed, "QUOTE_HEALTH_CONFIG_INVALID");
    return ErrorCode::kSystemError;
  }
  quote_health_monitor_.SetHealthChangedHandler([this](bool healthy) { OnMarketHealthChanged(healthy); });
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitEngineModules() {
  spdlog::info("InitEngineModules");

  EngineConfig runtime;
  {
    std::lock_guard lock(engine_config_mutex_);
    runtime = engine_config_;
  }

  // 1. OMS：仅内存状态机；冷启动不回放本地订单，Working 态由柜台快照对账重建
  oms::OrderManagerOptions order_options;
  order_options.engine_epoch = engine_epoch_;
  order_options.account_id = runtime.account_id;
  order_options.engine_id = runtime.engine_id;
  if (const auto rc = order_manager_.Initialize(order_options); rc != ErrorCode::kSuccess) {
    spdlog::error("order_manager init failed, code={}", static_cast<int>(rc));
    lifecycle_.Transition(EngineState::kFailed, "ORDER_MANAGER_INIT_FAILED");
    return rc;
  }

  // 2. EMS 注入 OMS；发送失败释放预占所需的 account-risk（与 Pipeline 对称）
  execution_manager_.SetOrderApi(&order_manager_);
  if (account_risk_bridge_ != nullptr) {
    order_pipeline_.SetAccountRiskBridge(account_risk_bridge_);
    order_pipeline_.SetAccountRiskIdentity(runtime.account_id, runtime.engine_id);
    execution_manager_.SetAccountRiskBridge(account_risk_bridge_);
    execution_manager_.SetAccountRiskIdentity(runtime.account_id);
  }

  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitEventLanes() {
  spdlog::info("InitEventLanes");
  // Lane-Q / Lane-T 对象在构造时已就绪；订阅与 reactor 线程在 StartEventLanes 完成
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitAdapters() {
  // 适配器由调用方注入并完成 Connect；可在 Init 之后、Start 之前补齐
  if (quote_api_ != nullptr && trader_api_ != nullptr) {
    return ErrorCode::kSuccess;
  }
  spdlog::info("adapters not yet injected at Init (SetQuoteApi/SetTraderApi before Start)");
  return ErrorCode::kSuccess;
}

// =============================================================================
// Start 子阶段
// =============================================================================

ErrorCode TradingEngine::StartEventLanes() {
  spdlog::info("StartEventLanes");
  // Stop() 会清空 Lane-T 订阅；每次 Start 前重新注册引擎级回报处理
  WireTraderEventHandlers();
  event_lanes_.Start();
  quote_health_monitor_.Start();
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::StartAdapters() {
  spdlog::info("StartAdapters");
  if (quote_api_ == nullptr || !quote_api_->IsConnected()) {
    lifecycle_.Transition(EngineState::kFailed, "QUOTE_NOT_CONNECTED");
    return ErrorCode::kConnectionError;
  }
  if (trader_api_ == nullptr || !trader_api_->IsConnected()) {
    lifecycle_.Transition(EngineState::kFailed, "TRADER_NOT_CONNECTED");
    return ErrorCode::kConnectionError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::ReconcileBrokerState() {
  spdlog::info("ReconcileBrokerState");
  // StartAdapters 已保证交易通道在线；查询柜台并对账，失败则拒绝 Start
  auto* trader_api = trader_api_.get();
  if (trader_api == nullptr || !trader_api->IsConnected()) {
    lifecycle_.Transition(EngineState::kFailed, "BROKER_RECONCILIATION_FAILED");
    return ErrorCode::kConnectionError;
  }

  // 1. 查询柜台订单、成交、持仓与资金
  qtrade::sdk::trader::QueryOrdersResponse orders_response;
  qtrade::sdk::trader::QueryTradesResponse trades_response;
  qtrade::sdk::trader::QueryPositionResponse positions_response;
  qtrade::sdk::trader::QueryAssetResponse asset_response;
  qtrade::sdk::trader::QueryAssetRequest asset_request;
  if (trader_api->QueryOrders({}, orders_response) != ErrorCode::kSuccess) {
    spdlog::error("query orders failed");
    lifecycle_.Transition(EngineState::kFailed, "BROKER_RECONCILIATION_FAILED");
    return ErrorCode::kInternalError;
  }

  if (trader_api->QueryTrades({}, trades_response) != ErrorCode::kSuccess) {
    spdlog::error("query trades failed");
    lifecycle_.Transition(EngineState::kFailed, "BROKER_RECONCILIATION_FAILED");
    return ErrorCode::kInternalError;
  }

  if (trader_api->QueryPositions({}, positions_response) != ErrorCode::kSuccess) {
    spdlog::error("query positions failed");
    lifecycle_.Transition(EngineState::kFailed, "BROKER_RECONCILIATION_FAILED");
    return ErrorCode::kInternalError;
  }

  asset_request.account_id = engine_config_.account_id;
  if (trader_api->QueryAsset(asset_request, asset_response) != ErrorCode::kSuccess) {
    spdlog::error("query asset failed");
    lifecycle_.Transition(EngineState::kFailed, "BROKER_RECONCILIATION_FAILED");
    return ErrorCode::kInternalError;
  }

  // 2. 应用柜台订单/成交并校验待对账订单已清空
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

  // 3. 校验待对账订单已清空
  const auto orders_requiring_reconciliation = order_manager_.GetOrdersRequiringReconciliation();
  if (!orders_requiring_reconciliation.empty()) {
    spdlog::error("orders requiring reconciliation not empty: {}", orders_requiring_reconciliation.size());
    lifecycle_.Transition(EngineState::kFailed, "BROKER_RECONCILIATION_FAILED");
    return ErrorCode::kInternalError;
  }
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
  if (subscribed_instruments_.empty()) {
    return ErrorCode::kSuccess;
  }
  SubscribeQuote(std::vector<std::string>{subscribed_instruments_.begin(), subscribed_instruments_.end()});
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
  quote_api_->SetTickCallback([this](const qtrade::sdk::quote::MarketTick& tick) {
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
  quote_api_->SetBarCallback([this](const qtrade::sdk::quote::Bar& bar) {
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
  trader_api_->SetOrderCallback([this](const qtrade::sdk::trader::Order& order) {
    if (!running_.load(std::memory_order_acquire) || !IsValidOrder(order)) {
      return;
    }
    event_lanes_.Trader().PublishOrder(order);
  });
  trader_api_->SetTradeCallback([this](const qtrade::sdk::trader::Trade& trade) {
    if (!running_.load(std::memory_order_acquire) || !IsValidTrade(trade)) {
      return;
    }
    event_lanes_.Trader().PublishTrade(trade);
  });
}

void TradingEngine::WireTraderEventHandlers() {
  // Trader Lane：订单/成交回报的唯一异步入口，串联 OMS、账户、持仓与 account-risk 释放
  event_lanes_.Trader().SubscribeOrder([this](const qtrade::sdk::trader::Order& order) { OnTraderOrderReport(order); });
  event_lanes_.Trader().SubscribeTrade([this](const qtrade::sdk::trader::Trade& trade) { OnTraderTradeReport(trade); });
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
// 运行时回调（配置应用 / 行情健康 → 生命周期）
// =============================================================================

ErrorCode TradingEngine::ApplyEngineConfig(const EngineConfig& config) {
  if (config.engine_id.empty() || config.account_id.empty()) {
    spdlog::warn("invalid engine config: empty engine_id/account_id");
    lifecycle_.Transition(EngineState::kFailed, "CONFIG_SNAPSHOT_INVALID");
    return ErrorCode::kInvalidArgument;
  }

  if (running_.load(std::memory_order_acquire)) {
    lifecycle_.Transition(EngineState::kFrozen, "CONFIG_RESTART_REQUIRED");
    spdlog::error("engine config cannot be applied while running; stop and re-init required");
    return ErrorCode::kAlreadyStarted;
  }

  {
    std::lock_guard lock(engine_config_mutex_);
    engine_config_ = config;
    // 仅 Init 路径重置订阅集；策略 CMS 由后续 AddStrategy 填充
    subscribed_instruments_.clear();
  }

  spdlog::info("applied engine config engine_id={}, account={}, quote_source={}",
               config.engine_id,
               config.account_id,
               config.quote_source);
  return ErrorCode::kSuccess;
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

void TradingEngine::OnTraderOrderReport(const qtrade::sdk::trader::Order& order) {
  order_manager_.ApplyOrderReport(order);
  const auto local_order = order.order_id.empty() ? order_manager_.GetOrderByClientId(order.client_order_id)
                                                  : order_manager_.GetOrder(order.order_id);
  if (local_order.has_value()) {
    account_manager_.ApplyOrder(*local_order);
  }

  // 拒单/撤单完成时释放 account-risk 预占（直接 gRPC，无本地 outbox）
  if (order.status != qtrade::sdk::trader::OrderStatusType::kRejected &&
      order.status != qtrade::sdk::trader::OrderStatusType::kCanceled) {
    return;
  }
  if (!local_order.has_value()) {
    return;
  }
  const auto reason = order.status == qtrade::sdk::trader::OrderStatusType::kCanceled
                        ? qtrade::account_risk::ReleaseReason::kCanceled
                        : qtrade::account_risk::ReleaseReason::kRejectedByVenue;
  ReleaseAccountRiskReservation(local_order->order_id, reason);
}

void TradingEngine::OnTraderTradeReport(const qtrade::sdk::trader::Trade& trade) {
  order_manager_.ApplyTradeReport(trade);
  account_manager_.ApplyTrade(trade);
  position_manager_.ApplyTrade(trade);

  // 全部成交后释放风控预占（SETTLED）
  const auto local_order = trade.order_id.empty() ? order_manager_.GetOrderByClientId(trade.client_order_id)
                                                  : order_manager_.GetOrder(trade.order_id);
  if (local_order.has_value() && local_order->status == qtrade::sdk::trader::OrderStatusType::kFilled) {
    ReleaseAccountRiskReservation(local_order->order_id, qtrade::account_risk::ReleaseReason::kSettled);
  }
}

void TradingEngine::ReleaseAccountRiskReservation(const std::string& order_id,
                                                  qtrade::account_risk::ReleaseReason reason) {
  if (account_risk_bridge_ == nullptr || order_id.empty()) {
    return;
  }
  std::string account_id;
  {
    std::lock_guard lock(engine_config_mutex_);
    account_id = engine_config_.account_id;
  }
  qtrade::account_risk::ReleaseRequest request;
  request.account_id = account_id;
  request.order_id = order_id;
  request.reason = reason;
  const auto result = account_risk_bridge_->Release(request);
  if (result.error_code != ErrorCode::kSuccess) {
    spdlog::warn("Release failed: order_id={}, code={}", order_id, static_cast<int>(result.error_code));
  }
}

qtrade::strategy::OrderSender TradingEngine::MakeOrderSender(std::string strategy_id) {
  return [this, strategy_id = std::move(strategy_id)](const qtrade::strategy::OrderBatch& batch) {
    if (!IsReady()) {
      return ErrorCode::kNotInitialized;
    }
    qtrade::strategy::OrderBatch stamped = batch;
    for (auto& request : stamped.order_requests) {
      if (request.strategy_id.empty()) {
        request.strategy_id = strategy_id;
      }
    }
    return order_pipeline_.SubmitBatch(stamped);
  };
}

std::unique_ptr<IEngine> CreateEngine() {
  return std::make_unique<TradingEngine>();
}

}  // namespace qtrade::engine
