/// @file      trading_engine.cpp
/// @brief     交易引擎实现
/// @details   实现 Init/Start/Stop、适配器装配、柜台对账、配置快照应用及发单撤单编排
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/trading_engine.hpp"

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

/// @brief 引擎内部固定的本地事实文件根目录
constexpr std::string_view kRuntimeDataDirectory = "data/";
/// @brief 首个进程世代
constexpr std::uint64_t kInitialEngineEpoch = 1;
/// @brief 订单与 outbox 事实必须同步落盘
constexpr bool kSyncOrderFacts = true;
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

[[nodiscard]] bool IsValidTrade(const qtrade_sdk::trader::Trade& trade) {
  return !trade.instrument.empty() && trade.volume > 0 && std::isfinite(trade.price) && trade.price >= 0.0 &&
         !(trade.order_id.empty() && trade.order_emt_id == 0 && trade.client_order_id == 0);
}

}  // namespace

TradingEngine::TradingEngine() : strategy_engine_(event_lanes_) {
  // 构造阶段完成三条主线装配：EMS↔OMS 回写、行情健康→READY 门禁、Lane-T→OMS/账户/持仓/风控释放
  // 1. 将 EMS 异步执行结果回写 OMS，并在入队失败时释放风控预占
  execution_manager_.SetResultHandlers(
    [this](const std::string& order_id) { return order_manager_.MarkSendPending(order_id); },
    [this](const std::string& order_id, ErrorCode result) {
      (void)order_manager_.RecordSendResult(order_id, result);
      if (result != ErrorCode::kSuccess && account_risk_client_.IsInitialized()) {
        (void)account_risk_release_worker_.Enqueue(order_id,
                                                   qtrade::account_risk::v1::ReleaseOrderRequest::EMS_ENQUEUE_FAILED);
      }
    },
    [this](const std::string& order_id, ErrorCode result) {
      (void)order_manager_.RecordCancelResult(order_id, result);
    });
  // 2. 将行情健康度与当前 OMS 状态接入 READY 门禁和风险计算
  quote_health_monitor_.SetHealthChangedHandler([this](bool healthy) { OnMarketHealthChanged(healthy); });
  risk_manager_.SetStateProviders([this] { return order_manager_.GetActiveOrderCount(); },
                                  [this] { return order_manager_.GetOpenNotional(); });

  // 3. Trader Lane：订单/成交回报的唯一异步入口，串联 OMS、账户、持仓与风控释放
  event_lanes_.Trader().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) {
    order_manager_.ApplyOrderReport(order);
    const auto local_order = order.order_id.empty() ? order_manager_.GetOrderByClientId(order.client_order_id)
                                                    : order_manager_.GetOrder(order.order_id);
    if (local_order.has_value()) {
      account_manager_.ApplyOrder(*local_order);
    }
    // 拒单/撤单完成时异步释放 account-risk 预占
    if (account_risk_client_.IsInitialized() && (order.status == qtrade_sdk::trader::OrderStatusType::kRejected ||
                                                 order.status == qtrade_sdk::trader::OrderStatusType::kCanceled)) {
      if (local_order.has_value()) {
        const auto reason = order.status == qtrade_sdk::trader::OrderStatusType::kCanceled
                              ? qtrade::account_risk::v1::ReleaseOrderRequest::CANCELED
                              : qtrade::account_risk::v1::ReleaseOrderRequest::REJECTED_BY_VENUE;
        (void)account_risk_release_worker_.Enqueue(local_order->order_id, reason);
      }
    }
  });
  event_lanes_.Trader().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) {
    order_manager_.ApplyTradeReport(trade);
    account_manager_.ApplyTrade(trade);
    position_manager_.ApplyTrade(trade);
    // 全部成交后释放风控预占（SETTLED）
    if (account_risk_client_.IsInitialized()) {
      const auto local_order = trade.order_id.empty() ? order_manager_.GetOrderByClientId(trade.client_order_id)
                                                      : order_manager_.GetOrder(trade.order_id);
      if (local_order.has_value() && local_order->status == qtrade_sdk::trader::OrderStatusType::kFilled) {
        (void)account_risk_release_worker_.Enqueue(local_order->order_id,
                                                   qtrade::account_risk::v1::ReleaseOrderRequest::SETTLED);
      }
    }
  });
}

TradingEngine::~TradingEngine() {
  // 1. 析构时确保运行态与 Init 侧资源均已释放
  Stop();
}

const qtrade::common::config::QtradeEngineConfig& TradingEngine::GetConfig() const {
  return config_;
}

event_bus::EventLanes& TradingEngine::GetEventLanes() {
  return event_lanes_;
}

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
    spdlog::warn("[TradingEngine] cannot subscribe quote: api not ready");
    return;
  }
  const auto rc = quote_api_->Subscribe({instruments});
  if (rc == ErrorCode::kSuccess) {
    spdlog::info("[TradingEngine] subscribed to {} instruments", instruments.size());
  } else {
    spdlog::error("[TradingEngine] quote subscription failed: {}", GetErrorCodeMessage(rc));
  }
}

void TradingEngine::UnsubscribeQuote(const std::vector<std::string>& instruments) {
  if (!running_ || quote_api_ == nullptr) {
    return;
  }
  quote_api_->Unsubscribe({instruments});
  spdlog::info("[TradingEngine] unsubscribed from {} instruments", instruments.size());
}

bool TradingEngine::IsQuoteHealthy() const {
  return quote_health_monitor_.IsHealthy();
}

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
      spdlog::warn("[TradingEngine] rejected invalid tick: instrument={}", tick.instrument);
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

strategy::StrategyEngine& TradingEngine::GetStrategyEngine() {
  return strategy_engine_;
}

client::LogClient& TradingEngine::GetLogClient() {
  return log_client_;
}

client::MonitorClient& TradingEngine::GetMonitorClient() {
  return monitor_client_;
}

client::ConfigClient& TradingEngine::GetConfigClient() {
  return config_client_;
}

oms::OrderManager& TradingEngine::GetOrderManager() {
  return order_manager_;
}

account::AccountManager& TradingEngine::GetAccountManager() {
  return account_manager_;
}

position::PositionManager& TradingEngine::GetPositionManager() {
  return position_manager_;
}

ErrorCode TradingEngine::Init(const qtrade::common::config::QtradeEngineConfig& config) {
  ErrorCode error_code = ErrorCode::kSuccess;
  // 1. 幂等：已 Init 则直接成功，避免重复装配 client 与围栏
  if (initialized_) {
    return error_code;
  }

  // Init-1: 引导配置与行情健康阈值
  error_code = BootstrapLocalRuntime(config);
  if (error_code != ErrorCode::kSuccess) {
    return error_code;
  }

  // Init-2: 单实例围栏
  error_code = AcquireInstanceFence();
  if (error_code != ErrorCode::kSuccess) {
    return error_code;
  }

  // Init-3: 订单事实回放
  error_code = ReplayLocalOrderFacts();
  if (error_code != ErrorCode::kSuccess) {
    ReleaseInitResources();
    return error_code;
  }

  // Init-4: D 段旁路
  error_code = InitBypassClients();
  if (error_code != ErrorCode::kSuccess) {
    ReleaseInitResources();
    return error_code;
  }

  // Init-5: 控制面 client
  error_code = InitControlPlaneClients();
  if (error_code != ErrorCode::kSuccess) {
    ReleaseInitResources();
    return error_code;
  }

  // Init-6: 按需连接适配器
  error_code = ConnectAdaptersIfConfigured();
  if (error_code != ErrorCode::kSuccess) {
    ReleaseInitResources();
    return error_code;
  }

  initialized_ = true;
  log_client_.Emit("info", "trading engine initialized");
  spdlog::info("[TradingEngine] Init pipeline completed, state={}", static_cast<int>(lifecycle_.State()));
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::BootstrapLocalRuntime(const qtrade::common::config::QtradeEngineConfig& config) {
  spdlog::info("[TradingEngine] Init-1 BootstrapLocalRuntime");
  // 1. 进入 Bootstrap 阶段，后续失败可据此定位到配置加载前
  if (lifecycle_.Advance(EngineLifecycleState::kBootstrap) != ErrorCode::kSuccess) {
    return ErrorCode::kSystemError;
  }

  // 2. 缓存进程引导配置，供身份校验与 client 地址解析
  config_ = config;
  // 3. 配置行情陈旧阈值，供 READY 门禁与 OnMarketHealthChanged 使用
  QuoteHealthOptions quote_health_options;
  quote_health_options.max_stale_age = kQuoteStaleThreshold;
  if (quote_health_monitor_.Configure(quote_health_options) != ErrorCode::kSuccess) {
    lifecycle_.Fail("QUOTE_HEALTH_CONFIG_INVALID");
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::AcquireInstanceFence() {
  spdlog::info("[TradingEngine] Init-2 AcquireInstanceFence");
  // 1. 按 tenant+account 写入围栏文件，防止同账户多实例并发写事实
  const std::string fence_path = std::string(kRuntimeDataDirectory) + config_.identity.tenant_id + "-" +
                                 config_.identity.account_id + "-engine.fence";
  if (const auto rc = engine_fence_.Acquire(fence_path, kInitialEngineEpoch); rc != ErrorCode::kSuccess) {
    lifecycle_.Fail("ENGINE_FENCE_ACQUIRE_FAILED");
    return rc;
  }
  // 2. 围栏成功后推进 kFenced；失败则释放围栏避免残留锁
  if (lifecycle_.Advance(EngineLifecycleState::kFenced) != ErrorCode::kSuccess) {
    lifecycle_.Fail("ENGINE_FENCE_FAILED");
    engine_fence_.Release();
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::ReplayLocalOrderFacts() {
  spdlog::info("[TradingEngine] Init-3 ReplayLocalOrderFacts");
  // 1. 打开订单 journal 并回放本地事实，epoch 与围栏绑定以拒绝陈旧写入
  const std::string journal_path = std::string(kRuntimeDataDirectory) + config_.identity.tenant_id + "-" +
                                   config_.identity.engine_id + "-orders.jsonl";
  oms::OrderManagerOptions order_options;
  order_options.tenant_id = config_.identity.tenant_id;
  order_options.engine_id = config_.identity.engine_id;
  order_options.engine_epoch = engine_fence_.Epoch();
  order_options.journal_path = journal_path;
  order_options.fsync_on_append = kSyncOrderFacts;
  if (const auto rc = order_manager_.Initialize(order_options); rc != ErrorCode::kSuccess) {
    spdlog::error("[TradingEngine] order journal init failed, code={}", static_cast<int>(rc));
    lifecycle_.Fail("ORDER_JOURNAL_INIT_FAILED");
    return rc;
  }
  // 2. 记录待柜台对账订单，Start 阶段 ReconcileBrokerState 会处理
  const auto unreconciled_orders = order_manager_.GetOrdersRequiringReconciliation();
  if (!unreconciled_orders.empty()) {
    spdlog::warn("[TradingEngine] {} orders require broker reconciliation", unreconciled_orders.size());
  }
  // 3. 回放完成方可进入 Start（生命周期要求 kReplayed）
  if (lifecycle_.Advance(EngineLifecycleState::kReplayed) != ErrorCode::kSuccess) {
    lifecycle_.Fail("ORDER_REPLAY_FAILED");
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitBypassClients() {
  spdlog::info("[TradingEngine] Init-4 InitBypassClients");
  // 1. D 段日志旁路：失败则 Init 终止，避免无观测的运行
  const auto log_topic = config_.support_services.log_service.Extension("topic").value_or("engine");
  if (const auto rc = log_client_.Init(log_topic); rc != ErrorCode::kSuccess) {
    spdlog::warn("[TradingEngine] log_client init failed, code={}", static_cast<int>(rc));
    lifecycle_.Fail("LOG_CLIENT_INIT_FAILED");
    return rc;
  }
  order_pipeline_.SetLogClient(&log_client_);

  // 2. D 段监控旁路（当前为本地 stub，后续可换真实 endpoint）
  if (const auto rc = monitor_client_.Init("stub://local"); rc != ErrorCode::kSuccess) {
    spdlog::warn("[TradingEngine] monitor_client init failed, code={}", static_cast<int>(rc));
    lifecycle_.Fail("MONITOR_CLIENT_INIT_FAILED");
    return rc;
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitControlPlaneClients() {
  spdlog::info("[TradingEngine] Init-5 InitControlPlaneClients");
  // 1. config-service：拉快照并 Watch，驱动 OnEngineConfig 应用运行时配置
  if (config_.support_services.config_service.enabled) {
    if (const auto rc = InitConfigClient(config_); rc != ErrorCode::kSuccess) {
      spdlog::error("[TradingEngine] config_client init failed, code={}", static_cast<int>(rc));
      lifecycle_.Fail("CONFIG_CLIENT_INIT_FAILED");
      return rc;
    }
  } else {
    spdlog::warn("[TradingEngine] config_service.enabled=false, skipping config_client");
  }

  // 2. account-risk：预占/释放 RPC + 可靠 outbox，供发单链与回报释放
  if (config_.support_services.account_risk_service.enabled) {
    if (const auto rc = InitAccountRiskClient(config_); rc != ErrorCode::kSuccess) {
      spdlog::error("[TradingEngine] account_risk_client init failed, code={}", static_cast<int>(rc));
      lifecycle_.Fail("ACCOUNT_RISK_CLIENT_INIT_FAILED");
      return rc;
    }
    order_pipeline_.SetAccountRiskClient(&account_risk_client_);
    const std::string release_outbox_path = std::string(kRuntimeDataDirectory) + config_.identity.tenant_id + "-" +
                                            config_.identity.engine_id + "-risk-release-outbox.jsonl";
    if (const auto rc =
          account_risk_release_worker_.Initialize(&account_risk_client_, release_outbox_path, kSyncOrderFacts);
        rc != ErrorCode::kSuccess) {
      spdlog::error("[TradingEngine] account risk release outbox init failed, code={}", static_cast<int>(rc));
      lifecycle_.Fail("ACCOUNT_RISK_OUTBOX_INIT_FAILED");
      return rc;
    }
    order_pipeline_.SetReleaseHandler([this](const std::string& order_id, int reason) {
      return account_risk_release_worker_.Enqueue(order_id, reason);
    });
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::ConnectAdaptersIfConfigured() {
  spdlog::info("[TradingEngine] Init-6 ConnectAdaptersIfConfigured");
  // 1. 未启用 config-service 时跳过（单元测试可注入 mock API）
  if (!config_.support_services.config_service.enabled) {
    spdlog::info("[TradingEngine] skip adapters (config_service disabled; tests may inject mock)");
    return ErrorCode::kSuccess;
  }
  // 2. 依赖 InitConfigClient 已拉取的 EngineConfig 快照连接适配器
  if (const auto rc = InitAdapters(); rc != ErrorCode::kSuccess) {
    spdlog::error("[TradingEngine] adapter init failed, code={}", static_cast<int>(rc));
    lifecycle_.Fail("ADAPTER_INIT_FAILED");
    return rc;
  }
  return ErrorCode::kSuccess;
}

void TradingEngine::ReleaseInitResources() {
  // 1. Init 失败或 Stop 未运行态：按依赖逆序释放，最后释放围栏
  account_risk_release_worker_.Stop();
  quote_health_monitor_.Stop();
  account_risk_client_.Shutdown();
  account_client_.Shutdown();
  config_client_.Shutdown();
  monitor_client_.Shutdown();
  log_client_.Shutdown();
  order_manager_.Shutdown();
  DisconnectAdapters();
  quote_api_.reset();
  trader_api_.reset();
  engine_fence_.Release();
  initialized_ = false;
}

ErrorCode TradingEngine::Start() {
  // 1. 前置校验：须 Init 完成且生命周期处于 kReplayed
  if (!initialized_) {
    spdlog::error("[TradingEngine] Init() must be called before Start()");
    return ErrorCode::kNotInitialized;
  }
  if (running_) {
    return ErrorCode::kSystemError;
  }
  if (lifecycle_.State() != EngineLifecycleState::kReplayed) {
    lifecycle_.Fail("INVALID_START_STATE");
    return ErrorCode::kSystemError;
  }

  // Start-1: 适配器就绪
  if (const auto rc = EnsureAdaptersConnected(); rc != ErrorCode::kSuccess) {
    return rc;
  }
  // Start-2: 柜台对账
  if (const auto rc = ReconcileBrokerState(); rc != ErrorCode::kSuccess) {
    return rc;
  }
  // Start-3: 运行时模块
  if (const auto rc = StartRuntimeModules(); rc != ErrorCode::kSuccess) {
    return rc;
  }
  // Start-4: 生命周期推进与 READY 门禁
  if (const auto rc = AdvancePostStartLifecycle(); rc != ErrorCode::kSuccess) {
    return rc;
  }

  monitor_client_.Counter("engine.start", 1.0);
  log_client_.Emit("info", "trading engine started");
  spdlog::info("[TradingEngine] Start pipeline completed, state={}, ready={}",
               static_cast<int>(lifecycle_.State()),
               lifecycle_.IsReady());
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::EnsureAdaptersConnected() {
  spdlog::info("[TradingEngine] Start-1 EnsureAdaptersConnected");
  // 1. 幂等装配并连接行情/交易 API（Init 阶段可能已连接）
  if (const auto result = InitAdapters(); result != ErrorCode::kSuccess) {
    lifecycle_.Fail("ADAPTER_NOT_READY");
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

ErrorCode TradingEngine::ReconcileBrokerState() {
  spdlog::info("[TradingEngine] Start-2 ReconcileBrokerState");
  auto* trader_api = trader_api_.get();
  const bool has_unreconciled_orders = !order_manager_.GetOrdersRequiringReconciliation().empty();
  // 1. 有待对账订单或策略要求快照时，查询柜台并合并到 OMS/Account/Position
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

ErrorCode TradingEngine::StartRuntimeModules() {
  spdlog::info("[TradingEngine] Start-3 StartRuntimeModules");

  // 1. 先启异步基础设施：risk outbox → 事件通道 → 健康监控 → 策略 → EMS
  if (account_risk_client_.IsInitialized()) {
    account_risk_release_worker_.Start();
  }
  event_lanes_.Start();
  quote_health_monitor_.Start();
  strategy_engine_.Start();
  execution_manager_.SetTraderApi(trader_api_.get());
  execution_manager_.Start();

  running_ = true;
  // 2. 按配置快照订阅行情（Init 期间 OnEngineConfig 可能已写入 subscribed_instruments_）
  {
    std::vector<std::string> instruments;
    {
      std::lock_guard lock(runtime_config_mutex_);
      instruments.assign(subscribed_instruments_.begin(), subscribed_instruments_.end());
    }
    if (!instruments.empty()) {
      SubscribeQuote(instruments);
    }
  }
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::AdvancePostStartLifecycle() {
  spdlog::info("[TradingEngine] Start-4 AdvancePostStartLifecycle");
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

ErrorCode TradingEngine::InitConfigClient(const qtrade::common::config::QtradeEngineConfig& config) {
  // 1. 校验 config-service 地址并初始化客户端
  const auto& config_service = config.support_services.config_service;
  if (!config_service.enabled || config_service.host.empty() || config_service.port <= 0) {
    return ErrorCode::kNotInitialized;
  }

  client::ConfigClientOptions client_opts;
  client_opts.server_address = config_service.Address();
  client_opts.tenant_id = config.identity.tenant_id;
  client_opts.engine_id = config.identity.engine_id;

  if (const auto rc = config_client_.Init(client_opts); rc != ErrorCode::kSuccess) {
    return rc;
  }

  // 2. 拉取快照并启动配置监视
  config_client_.SetOnSnapshot([this](const qtrade::config::v1::EngineConfig& config) { OnEngineConfig(config); });

  if (const auto rc = config_client_.FetchSnapshot(); rc != ErrorCode::kSuccess) {
    return rc;
  }

  if (const auto rc = config_client_.StartWatch(); rc != ErrorCode::kSuccess) {
    return rc;
  }

  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitAccountRiskClient(const qtrade::common::config::QtradeEngineConfig& config) {
  client::AccountRiskClientOptions options;
  options.server_address = config.support_services.account_risk_service.Address();
  options.tenant_id = config.identity.tenant_id;
  options.account_id = config.identity.account_id;
  options.engine_id = config.identity.engine_id;
  options.timeout_ms = config.support_services.account_risk_service.timeout_ms;
  return account_risk_client_.Init(options);
}

ErrorCode TradingEngine::InitAdapters() {
  // 1. 已装配则直接成功；否则读取运行时配置
  if (quote_api_ != nullptr && trader_api_ != nullptr) {
    return ErrorCode::kSuccess;
  }

  qtrade::config::v1::EngineConfig runtime_config;
  {
    std::lock_guard lock(runtime_config_mutex_);
    runtime_config = runtime_config_;
  }
  if (runtime_config.execution_adapter().empty() || runtime_config.quote_connection_string().empty()) {
    return ErrorCode::kNotInitialized;
  }

  // 2. 按适配器类型构造 API 与连接请求（mock / emt）
  qtrade_sdk::quote::ConnectRequest quote_request;
  qtrade_sdk::trader::ConnectRequest trader_request;
  if (runtime_config.execution_adapter() == "mock") {
    quote_request.name = "mock";
    quote_request.connection_string = runtime_config.quote_connection_string();
    trader_request.broker_id = "mock";
    trader_request.account_id = config_.identity.account_id;
    trader_request.connection_string = runtime_config.quote_connection_string();
    SetQuoteApi(qtrade::adapter::mock::quote::CreateMockQuoteApi());
    SetTraderApi(qtrade::adapter::mock::trader::CreateMockTraderApi());
  } else if (runtime_config.execution_adapter() == "emt") {
    const auto& account_service = config_.support_services.account_service;
    if (!account_service.enabled || account_service.host.empty() || account_service.port <= 0) {
      return ErrorCode::kNotInitialized;
    }
    client::AccountClientOptions options;
    options.server_address = account_service.Address();
    options.tenant_id = config_.identity.tenant_id;
    options.engine_id = config_.identity.engine_id;
    if (const auto result = account_client_.Init(options); result != ErrorCode::kSuccess) {
      return result;
    }
    qtrade::account::v1::GetCredentialResponse credential_response;
    if (const auto result = account_client_.GetCredential(config_.identity.account_id, credential_response);
        result != ErrorCode::kSuccess) {
      return result;
    }
    const auto& credential = credential_response.credential();
    if (credential.account_id() != config_.identity.account_id || credential.connection_string().empty() ||
        credential.password().empty()) {
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
    return ErrorCode::kNotSupported;
  }

  // 3. 连接行情与交易通道
  auto* quote_api = quote_api_.get();
  auto* trader_api = trader_api_.get();
  if (quote_api == nullptr || trader_api == nullptr) {
    return ErrorCode::kNotInitialized;
  }
  if (const auto result = quote_api->Connect(quote_request); result != ErrorCode::kSuccess) {
    quote_api->Disconnect();
    return result;
  }
  if (const auto result = trader_api->Connect(trader_request); result != ErrorCode::kSuccess) {
    quote_api->Disconnect();
    trader_api->Disconnect();
    return result;
  }
  return ErrorCode::kSuccess;
}

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
  asset_request.account_id = config_.identity.account_id;
  if (trader_api->QueryAsset(asset_request, asset_response) != ErrorCode::kSuccess) {
    return ErrorCode::kNotSupported;
  }

  // 2. 应用回报并校验待对账订单已清空
  for (const auto& report : orders_response.orders) {
    order_manager_.ApplyOrderReport(report);
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

void TradingEngine::OnEngineConfig(const qtrade::config::v1::EngineConfig& config) {
  // 1. 校验版本、身份与有效期；拒绝陈旧或运行中改适配器的快照
  if (config.version() == 0) {
    spdlog::warn("[TradingEngine] invalid engine config version={}", config.version());
    lifecycle_.Freeze("CONFIG_SNAPSHOT_INVALID");
    return;
  }

  const auto& engine = config;
  if (engine.engine_id() != config_.identity.engine_id || engine.tenant_id() != config_.identity.tenant_id ||
      engine.account_id() != config_.identity.account_id) {
    spdlog::error("[TradingEngine] config identity mismatch");
    lifecycle_.Freeze("CONFIG_IDENTITY_MISMATCH");
    return;
  }
  const auto now_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  if (engine.valid_until_unix_ms() > 0 && now_ms >= engine.valid_until_unix_ms()) {
    spdlog::error("[TradingEngine] rejected expired engine config version={}", config.version());
    lifecycle_.Freeze("CONFIG_EXPIRED");
    return;
  }

  {
    std::lock_guard lock(runtime_config_mutex_);
    if (config.version() <= runtime_config_version_) {
      spdlog::warn("[TradingEngine] ignored stale engine config version={}", config.version());
      return;
    }
    if (running_.load(std::memory_order_acquire) && runtime_config_version_ != 0 &&
        (runtime_config_.execution_adapter() != engine.execution_adapter() ||
         runtime_config_.quote_connection_string() != engine.quote_connection_string())) {
      lifecycle_.Freeze("ADAPTER_RESTART_REQUIRED");
      spdlog::error("[TradingEngine] adapter configuration changed while running; restart required");
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

  std::vector<strategy::StrategyRuntimeConfig> strategy_configs;
  strategy_configs.reserve(static_cast<std::size_t>(engine.strategies_size()));
  for (const auto& strategy : engine.strategies()) {
    strategy::StrategyRuntimeConfig config;
    config.strategy_id = strategy.strategy_id();
    config.plugin = strategy.plugin();
    config.enabled = strategy.enabled();
    config.instruments.assign(strategy.instruments().begin(), strategy.instruments().end());
    config.params.insert(strategy.params().begin(), strategy.params().end());
    strategy_configs.push_back(std::move(config));
  }
  if (strategy_engine_.ApplyConfiguration(strategy_configs) != ErrorCode::kSuccess) {
    lifecycle_.Freeze("STRATEGY_CONFIG_INVALID");
    return;
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

  spdlog::info("[TradingEngine] config snapshot version={}, account={}, quote_source={}, strategies={}",
               config.version(),
               config_.identity.account_id,
               engine.quote_source(),
               engine.strategies_size());

  for (const auto& strategy : engine.strategies()) {
    log_client_.Emit("info",
                     "strategy " + strategy.strategy_id() + " enabled=" + (strategy.enabled() ? "true" : "false"));
  }
}

void TradingEngine::OnMarketHealthChanged(bool healthy) {
  // 1. 行情不健康：READY/MarketHealthy → Frozen
  const auto state = lifecycle_.State();
  if (!healthy) {
    if (state == EngineLifecycleState::kReady || state == EngineLifecycleState::kMarketHealthy) {
      lifecycle_.Freeze("MARKET_UNHEALTHY");
      log_client_.Emit("warn", "engine frozen: market unhealthy");
    }
    return;
  }

  // 2. 行情恢复：推进 READY，或从 MARKET_UNHEALTHY 冻结恢复
  if (state == EngineLifecycleState::kRiskSynced) {
    if (lifecycle_.Advance(EngineLifecycleState::kMarketHealthy) == ErrorCode::kSuccess &&
        lifecycle_.Advance(EngineLifecycleState::kReady) == ErrorCode::kSuccess) {
      log_client_.Emit("info", "trading engine READY");
      monitor_client_.Counter("engine.ready", 1.0);
    }
  } else if (state == EngineLifecycleState::kFrozen && lifecycle_.Reason() == "MARKET_UNHEALTHY") {
    if (lifecycle_.ResumeReady() == ErrorCode::kSuccess) {
      log_client_.Emit("info", "trading engine resumed after market recovery");
    }
  }
}

ErrorCode TradingEngine::Stop() {
  // 1. 未运行：仅清理 Init 侧资源与围栏
  if (!running_) {
    if (initialized_) {
      ReleaseInitResources();
    } else {
      engine_fence_.Release();
    }
    lifecycle_.MarkStopped();
    return ErrorCode::kSystemError;
  }

  // 2. 排空：按依赖逆序停策略/行情/EMS/回报与旁路 client
  lifecycle_.BeginDrain();
  spdlog::info("[TradingEngine] stopping components...");

  running_ = false;
  strategy_engine_.Stop();
  quote_health_monitor_.Stop();
  execution_manager_.Stop();
  DisconnectAdapters();
  event_lanes_.Stop();
  account_risk_release_worker_.Stop();
  order_manager_.Shutdown();

  config_client_.Shutdown();
  account_client_.Shutdown();
  account_risk_client_.Shutdown();
  log_client_.Shutdown();
  monitor_client_.Shutdown();

  running_ = false;
  initialized_ = false;
  quote_api_.reset();
  trader_api_.reset();
  engine_fence_.Release();
  lifecycle_.MarkStopped();
  spdlog::info("[TradingEngine] stopped cleanly");
  return ErrorCode::kSuccess;
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

ErrorCode TradingEngine::SubmitOrder(const qtrade_sdk::trader::OrderRequest& request) {
  // 1. 仅 READY 门禁通过后才接受新单，流水线：CMS → Risk → OMS → EMS
  if (!lifecycle_.IsReady()) {
    return ErrorCode::kNotInitialized;
  }
  return order_pipeline_.Submit(request);
}

ErrorCode TradingEngine::CancelOrder(const std::string& order_id) {
  // 1. OMS 标记撤单意图
  const auto order = order_manager_.GetOrder(order_id);
  if (!order.has_value()) {
    return ErrorCode::kNotFound;
  }
  if (const auto result = order_manager_.CancelOrder(order_id); result != ErrorCode::kSuccess) {
    return result;
  }

  // 2. 入队 EMS 撤单；失败回写撤单结果
  qtrade_sdk::trader::CancelOrderRequest request;
  request.order_id = order_id;
  request.order_emt_id = order->order_emt_id;
  const auto result = execution_manager_.EnqueueCancel(request);
  if (result != ErrorCode::kSuccess) {
    (void)order_manager_.RecordCancelResult(order_id, result);
  }
  return result;
}

}  // namespace qtrade::engine
