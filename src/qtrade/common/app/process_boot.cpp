/// @file      process_boot.cpp
/// @brief     进程启动共用原语实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/process_boot.hpp"

#include "qtrade/common/logging/logger.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/common/system/systemd_notify.hpp"

#include <spdlog/spdlog.h>

#include <iostream>
#include <unistd.h>

namespace qtrade::common::process_boot {

bool ParseConfigPath(int argc, char** argv, std::string& config_path) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
      return true;
    }
  }
  return false;
}

bool InitProgramEnv(const std::string& service_name,
                    const std::string& log_dir,
                    const std::string& log_filename,
                    const std::string& config_path) {
  if (!InitSpdlogLogger(log_dir, log_filename)) {
    std::cerr << "failed to initialize logger, service_name=" << service_name << std::endl;
    return false;
  }

  spdlog::info("======================= Starting =========================");
  spdlog::info("[process_boot] InitProgramEnv");
  spdlog::info("service_name={}", service_name);
  spdlog::info("config_path={}", config_path);
  spdlog::info("service_pid={}", getpid());
  return true;
}

int WaitForInterruptAndNotifyStopping(const std::string& service_name) {
  spdlog::info("[{}] running until SIGINT/SIGTERM...", service_name);
  const int sig = system::WaitInterruptSignals();
  (void)system::NotifyStopping(service_name + " received signal " + std::to_string(sig));
  return sig;
}

void LogProcessStopped(const std::string& service_name) {
  spdlog::info("service_pid={}", getpid());
  spdlog::info("service_name={}", service_name);
  spdlog::info("[{}] stopped cleanly", service_name);
  spdlog::info("======================= Stopped =========================");
}

}  // namespace qtrade::common::process_boot
