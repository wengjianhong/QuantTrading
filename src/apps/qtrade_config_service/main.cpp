/// @file      main.cpp
/// @brief     配置中心服务（qtrade_config_service）
/// @details   gRPC GetConfig / WatchConfig 服务端
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "common/app/app_runner.hpp"
#include "common/logging/logger.hpp"
#include "service/config_service/config_server.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {

std::string ReadListenAddress(const std::string& config_path) {
  std::ifstream ifs(config_path);
  if (!ifs.is_open()) {
    return "0.0.0.0:50051";
  }
  nlohmann::json root;
  ifs >> root;
  if (root.contains("grpc") && root["grpc"].contains("listen")) {
    return root["grpc"]["listen"].get<std::string>();
  }
  return "0.0.0.0:50051";
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_path;
  if (!qtrade::common::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_config_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  if (!qtrade::common::init_spdlog_logger("logs", "config-service.log")) {
    return EXIT_FAILURE;
  }

  const auto context = qtrade::service::BootstrapConfigService(config_path);
  if (!context.repository) {
    spdlog::error("[qtrade_config_service] database not ready, check qtrade_config_service.json");
    return EXIT_FAILURE;
  }

  const std::string listen = ReadListenAddress(config_path);
  qtrade::service::ConfigServer server;
  if (const auto rc = server.Start(listen, context); rc != qtrade::ErrorCode::kSuccess) {
    spdlog::error("[qtrade_config_service] server start failed");
    return EXIT_FAILURE;
  }

  std::atomic<bool> stop_flag{false};
  qtrade::common::InstallShutdownHandler(stop_flag);

  std::thread grpc_thread([&server] { server.Wait(); });

  qtrade::common::UnblockShutdownSignals();
  spdlog::info("[qtrade_config_service] running until SIGINT/SIGTERM...");
  qtrade::common::RunUntilStop(stop_flag);

  server.Shutdown();
  if (grpc_thread.joinable()) {
    grpc_thread.join();
  }

  spdlog::info("[qtrade_config_service] stopped cleanly");
  return EXIT_SUCCESS;
}
