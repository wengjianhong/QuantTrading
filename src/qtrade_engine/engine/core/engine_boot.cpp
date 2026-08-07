/// @file      engine_boot.cpp
/// @brief     交易引擎进程启动阶段实现（业务相关）
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/engine_boot.hpp"

#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"
#include "qtrade/common/json/json_util.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/engine/trading_engine_define.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::engine::boot {

bool LoadStrategies(IEngine& engine, const std::string& plugin_dir) {
  spdlog::info("[engine_boot] LoadStrategies plugin_dir={}", plugin_dir);
  const ErrorCode code = engine.LoadStrategiesFromPlugins(plugin_dir);
  if (code != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] LoadStrategies failed, code={}", static_cast<int>(code));
    return false;
  }
  return true;
}

std::optional<qtrade::common::config::QtradeEngineBootstrapConfig> LoadBootstrapConfig(
  const qtrade::common::process_boot::ProgramOptions& options) {
  const auto config_node = qtrade::common::LoadJsonFile(options.config_path);
  if (!config_node) {
    spdlog::error("[engine_boot] LoadBootstrapConfig: failed to load config file");
    return std::nullopt;
  }
  const auto config = qtrade::common::config::ParseQtradeEngineBootstrapConfig(config_node.value());
  if (!config) {
    spdlog::error("[engine_boot] LoadBootstrapConfig: failed to parse config");
    return std::nullopt;
  }

  return config;
}

bool StartEngine(IEngine& engine) {
  spdlog::info("[engine_boot] StartEngine");
  const ErrorCode error_code = engine.Start();
  if (error_code != ErrorCode::kSuccess) {
    spdlog::error("[engine_boot] StartEngine failed, code={}", static_cast<int>(error_code));
    return false;
  }
  return true;
}

void RunUntilShutdown(IEngine& engine) {
  spdlog::info("[{}] running until SIGINT/SIGTERM...", qtrade::engine::kServiceName);

  const int signal = qtrade::common::system::WaitInterruptSignals();
  spdlog::info("[{}] received signal {}, stopping...", qtrade::engine::kServiceName, signal);

  (void)engine.Stop();
}

}  // namespace qtrade::engine::boot
