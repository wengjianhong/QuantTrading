/// @file      trading_engine.cpp
/// @brief     交易引擎实现
/// @details   实现顺序与 trading_engine.hpp 一致：构造析构 → 生命周期 → 交易入口 →
///            适配器/行情 → 模块访问器 → Init 子阶段 → Start 子阶段 → 接线/对账/回调。
///            EngineLifecycle 仅在本文件推进，不下沉到子模块。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/trading_engine.hpp"

#include "qtrade/error_code/error_codes.hpp"
#include "qtrade_sdk/emt/quote/emt_quote_api.hpp"
#include "qtrade_sdk/emt/trader/emt_trader_api.hpp"
#include "qtrade_sdk/mock/quote/mock_quote_api.hpp"
#include "qtrade_sdk/mock/trader/mock_trader_api.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.pb.h>
#include <qtrade/proto/config/v1/config.pb.h>

#include <spdlog/spdlog.h>

#include <chrono>
#include <cmath>
#include <unordered_set>
#include <vector>

namespace qtrade::engine {
namespace {

/// @brief 进程世代（写入 order_id；本地文件锁已移除，暂用固定值）
constexpr std::uint64_t kEngineEpoch = 1;
/// @brief 行情陈旧判定阈值
constexpr std::chrono::milliseconds kQuoteStaleThreshold = std::chrono::milliseconds(3000);
/// @brief 启动 READY 必须依赖已连接的交易通道
constexpr bool kRequireTraderConnection = true;
/// @brief 启动 READY 必须完成柜台快照同步
constexpr bool kRequireBrokerSnapshot = true;
/// @brief 默认必须等待有效行情后开放下单
constexpr bool kRequireMarketData = true;
/// @brief 未完成订单必须完成柜台对账
constexpr bool kAllowUnreconciledOrders = false;

[[nodiscard]] bool IsValidTick(const qtrade_sdk::quote::MarketTick& tick) {
  return !tick.instrument.empty() && tick.data_time > 0 && std::isfinite(tick.last_price) && tick.last_price > 0.0 &&
         tick.volume >= 0;
}

[[nodiscard]] bool IsValidBar(const qtrade_sdk::quote::Bar& bar) {
  return !bar.instrument.empty() && bar.open_time > 0 && bar.close_time >= bar.open_time && std::isfinite(bar.open) &&
         std::isfinite(bar.high) && std::isfinite(bar.low) && std::isfinite(bar.close) && bar.high >= bar.low &&
         bar.volume >= 0;
}

[[nodiscard]] bool IsValidOrder(const qtrade_sdk::trader::Order& order) {
  return !(order.order_id.empty() && order.order_emt_id == 0 && order.client_order_id == 0) && order.volume >= 0 &&
         order.traded_volume >= 0 && order.left_volume >= 0 &&
         !(order.volume > 0 && order.traded_volume > order.volume);
}

[[nodiscard]] bool StrategiesEqual(const google::protobuf::RepeatedPtrField<qtrade::config::v1::StrategyConfig>& lhs,
                                   const google::protobuf::RepeatedPtrField<qtrade::config::v1::StrategyConfig>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (int i = 0; i < lhs.size(); ++i) {
    const auto& a = lhs.Get(i);
    const auto& b = rhs.Get(i);
    if (a.strategy_id() != b.strategy_id() || a.strategy_name() != b.strategy_name() || a.enabled() != b.enabled() ||
        a.instruments_size() != b.instruments_size() || a.order_volume() != b.order_volume() ||
        a.max_position_volume() != b.max_position_volume() || a.order_cooldown_ms() != b.order_cooldown_ms() ||
        a.has_window_size() != b.has_window_size() || a.has_order_threshold() != b.has_order_threshold() ||
        a.has_stop_loss_percent() != b.has_stop_loss_percent() ||
        a.has_take_profit_percent() != b.has_take_profit_percent()) {
      return false;
    }
    if (a.has_window_size() && a.window_size() != b.window_size()) {
      return false;
    }
    if (a.has_order_threshold() && a.order_threshold() != b.order_threshold()) {
      return false;
    }
    if (a.has_stop_loss_percent() && a.stop_loss_percent() != b.stop_loss_percent()) {
      return false;
    }
    if (a.has_take_profit_percent() && a.take_profit_percent() != b.take_profit_percent()) {
      return false;
    }
    for (int j = 0; j < a.instruments_size(); ++j) {
      if (a.instruments(j) != b.instruments(j)) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] bool IsValidTrade(const qtrade_sdk::trader::Trade& trade) {
  return !trade.instrument.empty() && trade.volume > 0 && std::isfinite(trade.price) && trade.price >= 0.0 &&
         !(trade.order_id.empty() && trade.order_emt_id == 0 && trade.client_order_id == 0);
}

}  // namespace

// =============================================================================
// 构造 / 析构
// =============================================================================

TradingEngine::TradingEngine() : strategy_manager_(event_lanes_) {
  // 构造阶段装配：行情健康→READY 门禁、Risk 读 OMS、Lane-T→OMS/账户/持仓/风控释放
  // EMS↔OMS 回写在 InitEngineModules 中通过 SetOrderApi 注入，不再使用句柄
  // 1. 将行情健康度与当前 OMS 状态接入 READY 门禁和风险计算
  quote_health_monitor_.SetHealthChangedHandler([this](bool healthy) { OnMarketHealthChanged(healthy); });
  risk_manager_.SetStateProviders([this] { return order_manager_.GetActiveOrderCount(); },
                                  [this] { return order_manager_.GetOpenNotional(); });

  // 2. Trader Lane：订单/成交回报的唯一异步入口，串联 OMS、账户、持仓与风控释放
  event_lanes_.Trader().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) {
    order_manager_.ApplyOrderReport(order);
    const auto local_order = order.order_id.empty() ? order_manager_.GetOrderByClientId(order.client_order_id)
                                                    : order_manager_.GetOrder(order.order_id);
    if (local_order.has_value()) {
      account_manager_.ApplyOrder(*local_order);
    }
    // 拒单/撤单完成时释放 account-risk 预占（直接 gRPC，无本地 outbox）
    if (order.status == qtrade_sdk::trader::OrderStatusType::kRejected ||
        order.status == qtrade_sdk::trader::OrderStatusType::kCanceled) {
      if (local_order.has_value()) {
        const auto reason = order.status == qtrade_sdk::trader::OrderStatusType::kCanceled
                              ? qtrade::account_risk::v1::ReleaseOrderRequest::CANCELED
                              : qtrade::account_risk::v1::ReleaseOrderRequest::REJECTED_BY_VENUE;
        ReleaseAccountRiskReservation(local_order->order_id, reason);
      }
    }
  });
  event_lanes_.Trader().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) {
    order_manager_.ApplyTradeReport(trade);
    account_manager_.ApplyTrade(trade);
    position_manager_.ApplyTrade(trade);
    // 全部成交后释放风控预占（SETTLED）
    const auto local_order = trade.order_id.empty() ? order_manager_.GetOrderByClientId(trade.client_order_id)
                                                    : order_manager_.GetOrder(trade.order_id);
    if (local_order.has_value() && local_order->status == qtrade_sdk::trader::OrderStatusType::kFilled) {
      ReleaseAccountRiskReservation(local_order->order_id, qtrade::account_risk::v1::ReleaseOrderRequest::SETTLED);
    }
  });
}

TradingEngine::~TradingEngine() {
  // 1. 析构时确保运行态与 Init 侧资源均已释放
  Stop();
}

// =============================================================================
// 生命周期：Init → Start → Stop，以及状态查询
// =============================================================================

ErrorCode TradingEngine::Init(const qtrade::common::config::QtradeEngineBootstrapConfig& config) {
  if (initialized_) {
    return ErrorCode::kSuccess;
  }

  // 1. 引导配置与行情健康阈值 → kBootstrap
  if (const ErrorCode code = ApplyBootstrapConfig(config); code != ErrorCode::kSuccess) {
    spdlog::error("ApplyBootstrapConfig failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 2. 支撑服务客户端（config / account / account_risk 建连）
  if (const ErrorCode code = InitSupportClients(); code != ErrorCode::kSuccess) {
    spdlog::error("InitSupportClients failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 3. 拉取引擎运行配置 → runtime_config_（策略实例在 boot 按此加载）
  if (const ErrorCode code = FetchRuntimeConfig(); code != ErrorCode::kSuccess) {
    spdlog::error("FetchRuntimeConfig failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  // 4. 引擎内模块（内存 OMS 等）→ kModulesReady
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

  // 6. 行情/交易适配器
  if (const ErrorCode code = InitAdapters(); code != ErrorCode::kSuccess) {
    spdlog::error("InitAdapters failed, code={}", static_cast<int>(code));
    Release();
    return code;
  }

  initialized_ = true;
  spdlog::info("Init pipeline completed, state={}", static_cast<int>(lifecycle_.State()));
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::Start() {
  // 前置校验：须 Init 完成且生命周期处于 kModulesReady
  if (!initialized_) {
    spdlog::error("Init TradingEngine must be called before Start TradingEngine.");
    return ErrorCode::kNotInitialized;
  }
  if (running_) {
    spdlog::error("TradingEngine is already running.");
    return ErrorCode::kSystemError;
  }
  if (lifecycle_.State() != EngineLifecycleState::kModulesReady) {
    lifecycle_.Fail("INVALID_START_STATE");
    return ErrorCode::kSystemError;
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

  spdlog::info(
    "Start pipeline completed, state={}, ready={}", static_cast<int>(lifecycle_.State()), lifecycle_.IsReady());
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::Stop() {
  const bool was_running = running_.load(std::memory_order_acquire);
  if (was_running) {
    lifecycle_.BeginDrain();
    running_.store(false, std::memory_order_release);
    spdlog::info("stopping components...");
  }

  Release();
  initialized_.store(false, std::memory_order_release);
  lifecycle_.MarkStopped();

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

  // 2. 释放支撑服务客户端
  config_client_.Shutdown();
  account_client_.Shutdown();
  account_risk_client_.Shutdown();

  // 3. 释放行情/交易适配器
  quote_api_.reset();
  trader_api_.reset();
}

bool TradingEngine::IsRunning() const {
  return running_;
}

bool TradingEngine::IsReady() const {
  return lifecycle_.IsReady();
}

EngineLifecycleState TradingEngine::LifecycleState() const {
  return lifecycle_.State();
}

const qtrade::common::config::QtradeEngineBootstrapConfig& TradingEngine::GetConfig() const {
  return bootstrap_config_;
}

qtrade::config::v1::EngineConfig TradingEngine::GetRuntimeConfig() const {
  std::lock_guard lock(runtime_config_mutex_);
  return runtime_config_;
}

// =============================================================================
// 发单流水线访问
// =============================================================================

OrderPipeline& TradingEngine::GetOrderPipeline() {
  return order_pipeline_;
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

client::ConfigClient& TradingEngine::GetConfigClient() {
  return config_client_;
}

// =============================================================================
// Init 子阶段
// =============================================================================

ErrorCode TradingEngine::ApplyBootstrapConfig(const qtrade::common::config::QtradeEngineBootstrapConfig& config) {
  spdlog::info("ApplyBootstrapConfig");
  // 1. 进入 Bootstrap；后续失败可据此定位到配置加载前
  if (lifecycle_.Advance(EngineLifecycleState::kBootstrap) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }

  // 2. 缓存进程引导配置，供身份校验与 client 地址解析
  bootstrap_config_ = config;

  // 3. 配置行情陈旧阈值，供 READY 门禁与 OnMarketHealthChanged 使用
  QuoteHealthOptions quote_health_options;
  quote_health_options.max_stale_age = kQuoteStaleThreshold;
  if (quote_health_monitor_.Configure(quote_health_options) != ErrorCode::kSuccess) {
    lifecycle_.Fail("QUOTE_HEALTH_CONFIG_INVALID");
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitSupportClients() {
  spdlog::info("InitSupportClients");

  // 1. config-service：仅建连；拉配置在 FetchRuntimeConfig
  if (bootstrap_config_.support_services.config_service.enabled) {
    if (const auto rc = InitConfigClient(bootstrap_config_); rc != ErrorCode::kSuccess) {
      spdlog::error("config_client init failed, code={}", static_cast<int>(rc));
      lifecycle_.Fail("CONFIG_CLIENT_INIT_FAILED");
      return rc;
    }
  } else {
    spdlog::warn("config_service.enabled=false, skipping config_client");
  }

  // 2. account-service：仅建立通道；凭证在 InitAdapters（emt）按需 GetCredential
  if (bootstrap_config_.support_services.account_service.enabled) {
    client::AccountClientOptions options;
    options.service_config = bootstrap_config_.support_services.account_service;
    if (const auto rc = account_client_.Init(options); rc != ErrorCode::kSuccess) {
      spdlog::error("account_client init failed, code={}", static_cast<int>(rc));
      lifecycle_.Fail("ACCOUNT_CLIENT_INIT_FAILED");
      return rc;
    }
  } else {
    spdlog::warn("account_service.enabled=false, skipping account_client");
  }

  // 3. account-risk-service：仅建立通道；pipeline 接线在 InitEngineModules
  if (bootstrap_config_.support_services.account_risk_service.enabled) {
    if (const auto rc = InitAccountRiskClient(bootstrap_config_); rc != ErrorCode::kSuccess) {
      spdlog::error("account_risk_client init failed, code={}", static_cast<int>(rc));
      lifecycle_.Fail("ACCOUNT_RISK_CLIENT_INIT_FAILED");
      return rc;
    }
  } else {
    spdlog::warn("account_risk_service.enabled=false, skipping account_risk_client");
  }

  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitEngineModules() {
  spdlog::info("InitEngineModules");

  // 1. OMS：仅内存状态机；冷启动不回放本地订单，Working 态由柜台快照对账重建
  oms::OrderManagerOptions order_options;
  order_options.tenant_id = bootstrap_config_.config.identity.tenant_id;
  order_options.engine_id = bootstrap_config_.config.identity.engine_id;
  order_options.engine_epoch = kEngineEpoch;
  if (const auto rc = order_manager_.Initialize(order_options); rc != ErrorCode::kSuccess) {
    spdlog::error("order_manager init failed, code={}", static_cast<int>(rc));
    lifecycle_.Fail("ORDER_MANAGER_INIT_FAILED");
    return rc;
  }

  // 2. EMS 注入 OMS；发送失败释放预占所需的 account-risk（与 Pipeline 对称）
  execution_manager_.SetOrderApi(&order_manager_);
  if (account_risk_client_.IsInitialized()) {
    order_pipeline_.SetAccountRiskClient(&account_risk_client_);
    order_pipeline_.SetAccountRiskIdentity(bootstrap_config_.config.identity.tenant_id,
                                           bootstrap_config_.config.identity.account_id,
                                           bootstrap_config_.config.identity.engine_id);
    execution_manager_.SetAccountRiskClient(&account_risk_client_);
    execution_manager_.SetAccountRiskIdentity(bootstrap_config_.config.identity.tenant_id,
                                              bootstrap_config_.config.identity.account_id);
  }

  // 3. 模块初始化完成 → 允许进入 Start（kModulesReady）
  if (lifecycle_.Advance(EngineLifecycleState::kModulesReady) != ErrorCode::kSuccess) {
    lifecycle_.Fail("ENGINE_MODULES_INIT_FAILED");
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitEventLanes() {
  spdlog::info("InitEventLanes");
  // Lane-Q / Lane-T 在构造时已就绪；reactor 线程在 StartEventLanes 启动
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitAdapters() {
  // 1. 已装配则直接成功；config 未启用时跳过（单元测试可注入 mock API）
  if (quote_api_ != nullptr && trader_api_ != nullptr) {
    return ErrorCode::kSuccess;
  }
  if (!bootstrap_config_.support_services.config_service.enabled) {
    spdlog::info("skip adapters (config_service disabled; tests may inject mock)");
    return ErrorCode::kSuccess;
  }

  qtrade::config::v1::EngineConfig runtime_config;
  {
    std::lock_guard lock(runtime_config_mutex_);
    runtime_config = runtime_config_;
  }
  if (runtime_config.execution_adapter().empty() || runtime_config.quote_connection_string().empty()) {
    lifecycle_.Fail("ADAPTER_INIT_FAILED");
    return ErrorCode::kNotInitialized;
  }

  // 2. 按适配器类型构造 API 与连接请求（mock / emt）
  qtrade_sdk::quote::ConnectRequest quote_request;
  qtrade_sdk::trader::ConnectRequest trader_request;
  if (runtime_config.execution_adapter() == "mock") {
    quote_request.name = "mock";
    quote_request.connection_string = runtime_config.quote_connection_string();
    trader_request.broker_id = "mock";
    trader_request.account_id = bootstrap_config_.config.identity.account_id;
    trader_request.connection_string = runtime_config.quote_connection_string();
    SetQuoteApi(qtrade::adapter::mock::quote::CreateMockQuoteApi());
    SetTraderApi(qtrade::adapter::mock::trader::CreateMockTraderApi());
  } else if (runtime_config.execution_adapter() == "emt") {
    if (!account_client_.IsInitialized()) {
      lifecycle_.Fail("ADAPTER_INIT_FAILED");
      return ErrorCode::kNotInitialized;
    }
    qtrade::account::v1::GetCredentialRequest credential_request;
    credential_request.set_tenant_id(bootstrap_config_.config.identity.tenant_id);
    credential_request.set_engine_id(bootstrap_config_.config.identity.engine_id);
    credential_request.set_account_id(bootstrap_config_.config.identity.account_id);
    qtrade::account::v1::GetCredentialResponse credential_response;
    if (const auto result = account_client_.GetCredential(credential_request, credential_response);
        result != ErrorCode::kSuccess) {
      lifecycle_.Fail("ADAPTER_INIT_FAILED");
      return result;
    }
    const auto& credential = credential_response.credential();
    if (credential.account_id() != bootstrap_config_.config.identity.account_id ||
        credential.connection_string().empty() || credential.password().empty()) {
      lifecycle_.Fail("ADAPTER_INIT_FAILED");
      return ErrorCode::kInternalError;
    }
    trader_request.broker_id = credential.broker_id();
    trader_request.account_id = credential.account_id();
    trader_request.connection_string = credential.connection_string();
    trader_request.password = credential.password();
    quote_request.name = "emt";
    quote_request.connection_string = runtime_config.quote_connection_string();
    quote_request.user = credential.account_id();
    quote_request.password = credential.password();
    SetQuoteApi(std::make_unique<qtrade::adapter::quote::EmtQuoteApi>());
    SetTraderApi(std::make_unique<qtrade::adapter::trader::EmtTraderApi>());
  } else {
    lifecycle_.Fail("ADAPTER_INIT_FAILED");
    return ErrorCode::kNotSupported;
  }

  // 3. 连接行情与交易通道
  auto* quote_api = quote_api_.get();
  auto* trader_api = trader_api_.get();
  if (quote_api == nullptr || trader_api == nullptr) {
    lifecycle_.Fail("ADAPTER_INIT_FAILED");
    return ErrorCode::kNotInitialized;
  }
  if (const auto result = quote_api->Connect(quote_request); result != ErrorCode::kSuccess) {
    quote_api->Disconnect();
    lifecycle_.Fail("ADAPTER_INIT_FAILED");
    return result;
  }
  if (const auto result = trader_api->Connect(trader_request); result != ErrorCode::kSuccess) {
    quote_api->Disconnect();
    trader_api->Disconnect();
    lifecycle_.Fail("ADAPTER_INIT_FAILED");
    return result;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitConfigClient(const qtrade::common::config::QtradeEngineBootstrapConfig& config) {
  // 校验 config-service 地址并初始化客户端（不拉运行配置）
  const auto& config_service = config.support_services.config_service;
  if (!config_service.enabled || config_service.host.empty() || config_service.port <= 0) {
    return ErrorCode::kNotInitialized;
  }

  client::ConfigClientOptions client_opts;
  client_opts.service_config = config_service;
  return config_client_.Init(client_opts);
}

ErrorCode TradingEngine::FetchRuntimeConfig() {
  spdlog::info("FetchRuntimeConfig");
  if (!config_client_.IsInitialized()) {
    spdlog::warn("config_client not initialized, skip FetchRuntimeConfig");
    return ErrorCode::kSuccess;
  }

  // 1. 冷启动拉取引擎配置并应用（策略实例等工厂就绪后再 Apply）
  qtrade::config::v1::GetEngineConfigRequest get_request;
  get_request.set_engine_id(bootstrap_config_.config.identity.engine_id);
  qtrade::config::v1::GetEngineConfigResponse get_response;
  ErrorCode code = config_client_.GetEngineConfig(get_request, get_response);
  if (code != ErrorCode::kSuccess) {
    lifecycle_.Fail("GET_ENGINE_CONFIG_FAILED");
    return code;
  }
  OnEngineConfig(get_response.engine());

  // 2. 订阅热更新
  qtrade::config::v1::SubscribeEngineConfigRequest subscribe_request;
  subscribe_request.set_engine_id(bootstrap_config_.config.identity.engine_id);
  subscribe_request.set_since_version(get_response.engine().version());
  client::ConfigClient::SubscribeHandler on_subscribe =
    [this](const qtrade::config::v1::SubscribeEngineConfigResponse& response) { OnEngineConfig(response.engine()); };
  code = config_client_.SubscribeEngineConfig(subscribe_request, std::move(on_subscribe));
  if (code != ErrorCode::kSuccess) {
    lifecycle_.Fail("SUBSCRIBE_ENGINE_CONFIG_FAILED");
    return code;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitAccountRiskClient(const qtrade::common::config::QtradeEngineBootstrapConfig& config) {
  client::AccountRiskClientOptions options;
  options.service_config = config.support_services.account_risk_service;
  return account_risk_client_.Init(options);
}

// =============================================================================
// Start 子阶段
// =============================================================================

ErrorCode TradingEngine::StartAdapters() {
  spdlog::info("StartAdapters");
  // 1. 幂等装配并连接行情/交易 API（Init 阶段可能已连接）
  if (const auto result = InitAdapters(); result != ErrorCode::kSuccess) {
    if (lifecycle_.State() != EngineLifecycleState::kFailed) {
      lifecycle_.Fail("ADAPTER_NOT_READY");
    }
    return result;
  }
  // 2. 交易通道必须在线，否则无法对账与发单
  auto* trader_api = trader_api_.get();
  if (kRequireTraderConnection && (trader_api == nullptr || !trader_api->IsConnected())) {
    lifecycle_.Fail("TRADER_NOT_CONNECTED");
    return ErrorCode::kConnectionError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::SyncBrokerSnapshot() {
  spdlog::info("SyncBrokerSnapshot");
  auto* trader_api = trader_api_.get();
  const bool has_unreconciled_orders = !order_manager_.GetOrdersRequiringReconciliation().empty();
  // 1. 有待对账订单或策略要求快照时，查询柜台并 Adopt 到 OMS/Account/Position
  if (trader_api != nullptr && (kRequireBrokerSnapshot || has_unreconciled_orders)) {
    const auto sync_result = SynchronizeBrokerState(trader_api);
    if (sync_result != ErrorCode::kSuccess && (kRequireBrokerSnapshot || !kAllowUnreconciledOrders)) {
      lifecycle_.Fail("BROKER_RECONCILIATION_FAILED");
      return sync_result;
    }
  } else if (has_unreconciled_orders && !kAllowUnreconciledOrders) {
    // 2. 无 trader 连接但本地仍有待对账订单，拒绝启动
    lifecycle_.Fail("BROKER_RECONCILIATION_REQUIRED");
    return ErrorCode::kNotInitialized;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::StartEventLanes() {
  spdlog::info("StartEventLanes");
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
  // BrokerSynced: 柜台快照已应用；RiskSynced: MVP 暂无独立 RPC 对账，保留阶段位供后续扩展
  if (lifecycle_.Advance(EngineLifecycleState::kBrokerSynced) != ErrorCode::kSuccess ||
      lifecycle_.Advance(EngineLifecycleState::kRiskSynced) != ErrorCode::kSuccess) {
    lifecycle_.Fail("STARTUP_STATE_TRANSITION_FAILED");
    return ErrorCode::kSystemError;
  }
  if (!kRequireMarketData) {
    OnMarketHealthChanged(true);
  } else if (quote_health_monitor_.IsHealthy()) {
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
// 运行时回调（配置热更新 / 行情健康 → 生命周期）
// =============================================================================

void TradingEngine::OnEngineConfig(const qtrade::config::v1::EngineConfig& config) {
  // 1. 校验版本、身份与有效期；拒绝陈旧或运行中改适配器的快照
  if (config.version() == 0) {
    spdlog::warn("invalid engine config version={}", config.version());
    lifecycle_.Freeze("CONFIG_SNAPSHOT_INVALID");
    return;
  }

  const auto& engine = config;
  if (engine.engine_id() != bootstrap_config_.config.identity.engine_id ||
      engine.tenant_id() != bootstrap_config_.config.identity.tenant_id ||
      engine.account_id() != bootstrap_config_.config.identity.account_id) {
    spdlog::error("config identity mismatch");
    lifecycle_.Freeze("CONFIG_IDENTITY_MISMATCH");
    return;
  }
  const auto now_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  if (engine.valid_until_unix_ms() > 0 && now_ms >= engine.valid_until_unix_ms()) {
    spdlog::error("rejected expired engine config version={}", config.version());
    lifecycle_.Freeze("CONFIG_EXPIRED");
    return;
  }

  {
    std::lock_guard lock(runtime_config_mutex_);
    if (config.version() <= runtime_config_version_) {
      spdlog::warn("ignored stale engine config version={}", config.version());
      return;
    }
    if (running_.load(std::memory_order_acquire) && runtime_config_version_ != 0 &&
        (runtime_config_.execution_adapter() != engine.execution_adapter() ||
         runtime_config_.quote_connection_string() != engine.quote_connection_string())) {
      lifecycle_.Freeze("ADAPTER_RESTART_REQUIRED");
      spdlog::error("adapter configuration changed while running; restart required");
      return;
    }
  }

  // 2. 应用风控、策略与合规规则
  risk::RiskLimits limits;
  if (engine.has_risk_budget()) {
    const auto& budget = engine.risk_budget();
    limits.version = budget.version() != 0 ? budget.version() : config.version();
    limits.max_order_notional = budget.max_notional();
    limits.max_total_notional = budget.max_notional();
    limits.max_open_orders = budget.max_open_orders();
    limits.safety_buffer = budget.safety_buffer();
  } else {
    limits.version = config.version();
  }
  if (risk_manager_.Configure(limits) != ErrorCode::kSuccess) {
    lifecycle_.Freeze("RISK_CONFIG_INVALID");
    return;
  }

  // 策略实例仅在 Init 后由 boot::LoadStrategies 装配；运行中变更须 Stop → 重新 Init
  if (running_.load(std::memory_order_acquire)) {
    bool strategies_changed = false;
    {
      std::lock_guard lock(runtime_config_mutex_);
      strategies_changed = !StrategiesEqual(runtime_config_.strategies(), engine.strategies());
    }
    if (strategies_changed) {
      lifecycle_.Freeze("STRATEGY_RESTART_REQUIRED");
      spdlog::error("strategy configuration changed while running; stop and re-init required");
      return;
    }
  } else {
    spdlog::info("cached {} strategy config(s); instances will be created by LoadStrategies", engine.strategies_size());
  }

  std::unordered_set<std::string> desired_instruments;
  for (const auto& strategy : engine.strategies()) {
    if (!strategy.enabled()) {
      continue;
    }
    for (const auto& instrument : strategy.instruments()) {
      if (!instrument.empty()) {
        desired_instruments.insert(instrument);
      }
    }
  }
  cms::ComplianceRules compliance_rules;
  compliance_rules.version = config.version();
  compliance_rules.allowed_instruments = desired_instruments;
  if (engine.has_risk_budget()) {
    compliance_rules.max_notional = engine.risk_budget().max_notional();
  }
  if (compliance_.Configure(compliance_rules) != ErrorCode::kSuccess) {
    lifecycle_.Freeze("COMPLIANCE_CONFIG_INVALID");
    return;
  }

  // 3. 更新订阅集合；运行中则增量订/退订
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
    runtime_config_ = engine;
    runtime_config_version_ = config.version();
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
               config.version(),
               bootstrap_config_.config.identity.account_id,
               engine.quote_source(),
               engine.strategies_size());

  for (const auto& strategy : engine.strategies()) {
    spdlog::info("strategy {} enabled={}", strategy.strategy_id(), strategy.enabled());
  }
}

void TradingEngine::OnMarketHealthChanged(bool healthy) {
  // 1. 行情不健康：READY/MarketHealthy → Frozen
  const auto state = lifecycle_.State();
  if (!healthy) {
    if (state == EngineLifecycleState::kReady || state == EngineLifecycleState::kMarketHealthy) {
      lifecycle_.Freeze("MARKET_UNHEALTHY");
      spdlog::warn("engine frozen: market unhealthy");
    }
    return;
  }

  // 2. 行情恢复：推进 READY，或从 MARKET_UNHEALTHY 冻结恢复
  if (state == EngineLifecycleState::kRiskSynced) {
    if (lifecycle_.Advance(EngineLifecycleState::kMarketHealthy) == ErrorCode::kSuccess &&
        lifecycle_.Advance(EngineLifecycleState::kReady) == ErrorCode::kSuccess) {
      spdlog::info("trading engine READY");
    }
  } else if (state == EngineLifecycleState::kFrozen && lifecycle_.Reason() == "MARKET_UNHEALTHY") {
    if (lifecycle_.ResumeReady() == ErrorCode::kSuccess) {
      spdlog::info("trading engine resumed after market recovery");
    }
  }
}

void TradingEngine::ReleaseAccountRiskReservation(const std::string& order_id,
                                                  qtrade::account_risk::v1::ReleaseOrderRequest::Reason reason) {
  if (!account_risk_client_.IsInitialized() || order_id.empty()) {
    return;
  }
  qtrade::account_risk::v1::ReleaseOrderRequest request;
  request.set_tenant_id(bootstrap_config_.config.identity.tenant_id);
  request.set_account_id(bootstrap_config_.config.identity.account_id);
  request.set_order_id(order_id);
  request.set_reason(reason);
  qtrade::account_risk::v1::ReleaseOrderResponse response;
  if (const auto rc = account_risk_client_.ReleaseOrder(request, response); rc != ErrorCode::kSuccess) {
    spdlog::warn("ReleaseOrder failed: order_id={}, code={}", order_id, static_cast<int>(rc));
  }
}

}  // namespace qtrade::engine
