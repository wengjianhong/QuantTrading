/// @file      trading_engine.cpp
/// @brief     交易引擎实现
/// @details   实现交易引擎的启动、停止及各子模块协调逻辑
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/trading_engine.hpp"

#include <qtrade/proto/config/v1/config.pb.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

namespace qtrade::engine {

namespace {

ErrorCode ParseEngineOptionsFromJson(const std::string& json_path, EngineOptions& options) {
  std::ifstream ifs(json_path);
  if (!ifs.is_open()) {
    spdlog::error("[TradingEngine] cannot open config: {}", json_path);
    return ErrorCode::kNotFound;
  }

  nlohmann::json root;
  try {
    ifs >> root;
  } catch (const nlohmann::json::exception& ex) {
    spdlog::error("[TradingEngine] invalid config JSON: {}", ex.what());
    return ErrorCode::kInternal;
  }

  if (root.contains("config_service")) {
    options.config_server_address = root["config_service"].get<std::string>();
  }
  if (root.contains("account_service")) {
    options.account_server_address = root["account_service"].get<std::string>();
  }
  if (root.contains("tenant_id")) {
    options.tenant_id = root["tenant_id"].get<std::string>();
  }
  if (root.contains("engine_id")) {
    options.engine_id = root["engine_id"].get<std::string>();
  }
  if (root.contains("account_id")) {
    options.account_id = root["account_id"].get<std::string>();
  }
  if (root.contains("log_topic")) {
    options.log_topic = root["log_topic"].get<std::string>();
  }
  if (root.contains("monitor_endpoint")) {
    options.monitor_endpoint = root["monitor_endpoint"].get<std::string>();
  }

  return ErrorCode::kSuccess;
}

}  // namespace

TradingEngine::TradingEngine()
  : strategy_engine_(event_lanes_),
    quote_normalizer_(event_lanes_.Market()),
    trader_normalizer_(event_lanes_.Return()) {}

TradingEngine::~TradingEngine() {
  Stop();
}

ErrorCode TradingEngine::ReloadFromJson(const std::string& json_path) {
  EngineOptions loaded;
  if (const auto rc = ParseEngineOptionsFromJson(json_path, loaded); rc != ErrorCode::kSuccess) {
    return rc;
  }
  options_ = std::move(loaded);
  spdlog::info("[TradingEngine] config loaded from {}", json_path);
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::Init() {
  return Init(options_);
}

ErrorCode TradingEngine::Init(const EngineOptions& options) {
  if (initialized_) {
    return ErrorCode::kSystemError;
  }

  options_ = options;

  if (const auto rc = log_client_.Init(options_.log_topic); rc != ErrorCode::kSuccess) {
    spdlog::warn("[TradingEngine] log_client init failed, code={}", static_cast<int>(rc));
    return rc;
  }

  const std::string monitor_endpoint = options_.monitor_endpoint.empty() ? "stub://local" : options_.monitor_endpoint;
  if (const auto rc = monitor_client_.Init(monitor_endpoint); rc != ErrorCode::kSuccess) {
    spdlog::warn("[TradingEngine] monitor_client init failed, code={}", static_cast<int>(rc));
    log_client_.Shutdown();
    return rc;
  }

  if (!options_.config_server_address.empty()) {
    if (const auto rc = InitConfigClient(options_); rc != ErrorCode::kSuccess) {
      spdlog::warn("[TradingEngine] config_client init failed, code={} (using local defaults)", static_cast<int>(rc));
    }
  } else {
    spdlog::info("[TradingEngine] config_server_address empty, skipping config_client");
  }

  initialized_ = true;
  log_client_.Emit("info", "trading engine initialized");
  return ErrorCode::kSuccess;
}

ErrorCode TradingEngine::InitConfigClient(const EngineOptions& options) {
  client::ConfigClientOptions client_opts;
  client_opts.server_address = options.config_server_address;
  client_opts.tenant_id = options.tenant_id;
  client_opts.engine_id = options.engine_id;

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

void TradingEngine::OnConfigSnapshot(const qtrade::config::v1::ConfigSnapshot& snapshot) {
  if (!snapshot.has_engine()) {
    spdlog::warn("[TradingEngine] config snapshot version={} has no engine config", snapshot.version());
    return;
  }

  const auto& engine = snapshot.engine();
  if (!engine.engine_id().empty() && engine.engine_id() != options_.engine_id) {
    spdlog::warn("[TradingEngine] engine_id mismatch: snapshot={} local={}", engine.engine_id(), options_.engine_id);
  }

  runtime_config_ = engine;
  spdlog::info("[TradingEngine] config snapshot version={}, account={}, quote_source={}, strategies={}",
               snapshot.version(),
               options_.account_id,
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

}  // namespace qtrade::engine
