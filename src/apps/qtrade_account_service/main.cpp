/// @file      main.cpp
/// @brief     交易账户服务（qtrade_account_service）
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "common/app/app_runner.hpp"
#include "common/logging/logger.hpp"
#include "service/account_service/account_server.hpp"

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
    return "0.0.0.0:50052";
  }
  nlohmann::json root;
  ifs >> root;
  if (root.contains("grpc") && root["grpc"].contains("listen")) {
    return root["grpc"]["listen"].get<std::string>();
  }
  return "0.0.0.0:50052";
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_path;
  if (!qtrade::common::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_account_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  if (!qtrade::common::init_spdlog_logger("logs", "account-service.log")) {
    return EXIT_FAILURE;
  }

  const auto context = qtrade::service::BootstrapAccountService(config_path);
  if (!context.repository) {
    spdlog::error("[qtrade_account_service] database not ready, check qtrade_account_service.json");
    return EXIT_FAILURE;
  }

  const std::string listen = ReadListenAddress(config_path);
  qtrade::service::AccountServer server;
  if (const auto rc = server.Start(listen, context); rc != qtrade::ErrorCode::kSuccess) {
    spdlog::error("[qtrade_account_service] server start failed");
    return EXIT_FAILURE;
  }

  std::atomic<bool> stop_flag{false};
  qtrade::common::InstallShutdownHandler(stop_flag);

  std::thread grpc_thread([&server] { server.Wait(); });

  qtrade::common::UnblockShutdownSignals();
  spdlog::info("[qtrade_account_service] running until SIGINT/SIGTERM...");
  qtrade::common::RunUntilStop(stop_flag);

  server.Shutdown();
  if (grpc_thread.joinable()) {
    grpc_thread.join();
  }

  spdlog::info("[qtrade_account_service] stopped cleanly");
  return EXIT_SUCCESS;
}
