/// @file      trading_engine.cpp
/// @brief     交易引擎实现
/// @details   实现交易引擎的启动、停止及各子模块协调逻辑
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/trading_engine.hpp"

#include "qtrade/common/file/text_file.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.pb.h>
#include <qtrade/proto/config/v1/config.pb.h>

#include <spdlog/spdlog.h>

namespace qtrade::engine {

TradingEngine::TradingEngine()
  : strategy_engine_(event_lanes_),
    quote_normalizer_(event_lanes_.Market()),
    trader_normalizer_(event_lanes_.Return()) {
  event_lanes_.Return().SubscribeOrder([this](const qtrade_sdk::trader::Order& order) {
    order_manager_.ApplyOrderReport(order);
    account_manager_.ApplyOrder(order);
    if (account_risk_client_.IsInitialized() && (order.status == qtrade_sdk::trader::OrderStatusType::kRejected ||
                                                 order.status == qtrade_sdk::trader::OrderStatusType::kCanceled)) {
      qtrade::account_risk::v1::ReleaseOrderResponse ignored;
      (void)account_risk_client_.ReleaseOrder(
        order.order_id, qtrade::account_risk::v1::ReleaseOrderRequest::REJECTED_BY_VENUE, ignored);
    }
  });
  event_lanes_.Return().SubscribeTrade([this](const qtrade_sdk::trader::Trade& trade) {
    order_manager_.ApplyTradeReport(trade);
    account_manager_.ApplyTrade(trade);
    position_manager_.ApplyTrade(trade);
    if (account_risk_client_.IsInitialized() && trade.order_id.size() > 0) {
      qtrade::account_risk::v1::ReleaseOrderResponse ignored;
      (void)account_risk_client_.ReleaseOrder(
        trade.order_id, qtrade::account_risk::v1::ReleaseOrderRequest::SETTLED, ignored);
    }
  });
}

TradingEngine::~TradingEngine() {
  Stop();
}

ErrorCode TradingEngine::ReloadFromJson(const std::string& json_path) {
  const auto json_text = qtrade::common::ReadTextFile(json_path);
  if (!json_text.has_value()) {
    return ErrorCode::kNotFound;
  }
  const auto loaded = qtrade::common::config::ParseQtradeEngineConfig(*json_text);
  if (!loaded.has_value()) {
    return ErrorCode::kNotFound;
  }
  config_ = *loaded;
  spdlog::info("[TradingEngine] config loaded from {}", json_path);
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::Init() {
  return Init(config_);
}

ErrorCode TradingEngine::Init(const qtrade::common::config::QtradeEngineConfig& config) {
  if (initialized_) {
    return ErrorCode::kSystemError;
  }

  config_ = config;

  if (const auto rc = log_client_.Init(config_.log_topic); rc != ErrorCode::kSuccess) {
    spdlog::warn("[TradingEngine] log_client init failed, code={}", static_cast<int>(rc));
    return rc;
  }
  order_pipeline_.SetLogClient(&log_client_);

  const std::string monitor_endpoint = config_.monitor_endpoint.empty() ? "stub://local" : config_.monitor_endpoint;
  if (const auto rc = monitor_client_.Init(monitor_endpoint); rc != ErrorCode::kSuccess) {
    spdlog::warn("[TradingEngine] monitor_client init failed, code={}", static_cast<int>(rc));
    log_client_.Shutdown();
    return rc;
  }

  if (!config_.config_service.empty()) {
    if (const auto rc = InitConfigClient(config_); rc != ErrorCode::kSuccess) {
      spdlog::warn("[TradingEngine] config_client init failed, code={} (using local defaults)", static_cast<int>(rc));
    }
  } else {
    spdlog::info("[TradingEngine] config_service empty, skipping config_client");
  }

  if (config_.account_risk_enabled) {
    if (const auto rc = InitAccountRiskClient(config_); rc != ErrorCode::kSuccess) {
      monitor_client_.Shutdown();
      log_client_.Shutdown();
      return rc;
    }
    order_pipeline_.SetAccountRiskClient(&account_risk_client_);
  }

  initialized_ = true;
  log_client_.Emit("info", "trading engine initialized");
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitConfigClient(const qtrade::common::config::QtradeEngineConfig& config) {
  client::ConfigClientOptions client_opts;
  client_opts.server_address = config.config_service;
  client_opts.tenant_id = config.tenant_id;
  client_opts.engine_id = config.engine_id;

  if (const auto rc = config_client_.Init(client_opts); rc != ErrorCode::kSuccess) {
    return rc;
  }

  config_client_.SetOnSnapshot(
    [this](const qtrade::config::v1::ConfigSnapshot& snapshot) { OnConfigSnapshot(snapshot); });

  if (const auto rc = config_client_.FetchSnapshot(); rc != ErrorCode::kSuccess) {
    spdlog::warn("[TradingEngine] GetConfig failed, continuing with local snapshot");
  }

  if (const auto rc = config_client_.StartWatch(); rc != ErrorCode::kSuccess) {
    return rc;
  }

  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitAccountRiskClient(const qtrade::common::config::QtradeEngineConfig& config) {
  client::AccountRiskClientOptions options;
  options.server_address = config.account_risk_service;
  options.tenant_id = config.tenant_id;
  options.account_id = config.account_id;
  options.engine_id = config.engine_id;
  options.timeout_ms = config.account_risk_timeout_ms;
  return account_risk_client_.Init(options);
}

void TradingEngine::OnConfigSnapshot(const qtrade::config::v1::ConfigSnapshot& snapshot) {
  if (!snapshot.has_engine()) {
    spdlog::warn("[TradingEngine] config snapshot version={} has no engine config", snapshot.version());
    return;
  }

  const auto& engine = snapshot.engine();
  if (!engine.engine_id().empty() && engine.engine_id() != config_.engine_id) {
    spdlog::warn("[TradingEngine] engine_id mismatch: snapshot={} local={}", engine.engine_id(), config_.engine_id);
  }

  runtime_config_ = engine;
  spdlog::info("[TradingEngine] config snapshot version={}, account={}, quote_source={}, strategies={}",
               snapshot.version(),
               config_.account_id,
               engine.quote_source(),
               engine.strategies_size());

  for (const auto& strategy : engine.strategies()) {
    log_client_.Emit("info",
                     "strategy " + strategy.strategy_id() + " enabled=" + (strategy.enabled() ? "true" : "false"));
  }
}

ErrorCode TradingEngine::Start() {
  if (!initialized_) {
    spdlog::error("[TradingEngine] Init() must be called before Start()");
    return ErrorCode::kNotInitialized;
  }
  if (running_) {
    return ErrorCode::kSystemError;
  }

  spdlog::info("[TradingEngine] starting components...");

  event_lanes_.Start();
  quote_normalizer_.Start();
  trader_normalizer_.Start();
  strategy_engine_.Start();
  compliance_.Start();
  order_manager_.Start();
  account_manager_.Start();
  position_manager_.Start();
  risk_manager_.Start();
  execution_manager_.SetTraderApi(trader_normalizer_.GetTraderApi());
  execution_manager_.Start();

  running_ = true;
  monitor_client_.Counter("engine.start", 1.0);
  log_client_.Emit("info", "trading engine started");
  spdlog::info("[TradingEngine] started successfully");
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::Stop() {
  if (!running_) {
    if (initialized_) {
      config_client_.Shutdown();
      account_risk_client_.Shutdown();
      log_client_.Shutdown();
      monitor_client_.Shutdown();
      initialized_ = false;
    }
    return ErrorCode::kSystemError;
  }

  spdlog::info("[TradingEngine] stopping components...");

  execution_manager_.Stop();
  risk_manager_.Stop();
  position_manager_.Stop();
  account_manager_.Stop();
  order_manager_.Stop();
  compliance_.Stop();
  strategy_engine_.Stop();
  trader_normalizer_.Stop();
  quote_normalizer_.Stop();
  event_lanes_.Stop();

  config_client_.Shutdown();
  account_risk_client_.Shutdown();
  log_client_.Shutdown();
  monitor_client_.Shutdown();

  running_ = false;
  initialized_ = false;
  spdlog::info("[TradingEngine] stopped cleanly");
  return ErrorCode::kSuccess;
}

bool TradingEngine::IsRunning() const {
  return running_;
}

ErrorCode TradingEngine::SubmitOrder(const qtrade_sdk::trader::OrderRequest& request) {
  return order_pipeline_.Submit(request);
}

}  // namespace qtrade::engine
