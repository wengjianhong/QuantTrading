/// @file      app_runner.cpp
/// @brief     应用启动器实现
/// @details   实现信号处理、配置路径解析及服务进程通用主循环
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "common/app/app_runner.hpp"
#include "common/logging/logger.hpp"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

namespace qtrade::common {

namespace {

/// 进程内退出标志；信号 handler 写入，RunUntilStop 读取并同步到调用方 stop_flag
std::atomic<bool> g_stop_requested = false;

/// @brief SIGINT/SIGTERM 信号 handler
/// @details async-signal-safe：仅 atomic store，禁止日志、锁、堆分配等操作
void OnShutdownSignal(int signum) {
  (void)signum;
  g_stop_requested.store(true, std::memory_order_release);
}

/// @brief 对当前线程应用 SIGINT/SIGTERM 屏蔽或恢复
/// @param operation pthread_sigmask 操作码，如 SIG_BLOCK / SIG_UNBLOCK
void ApplySignalMask(int operation) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);

  if (pthread_sigmask(operation, &set, nullptr) != 0) {
    spdlog::error("[app_runner] pthread_sigmask failed: {}", std::strerror(errno));
  }
}

}  // namespace

void InstallShutdownHandler(std::atomic<bool>& stop_flag) {
  g_stop_requested.store(false, std::memory_order_release);
  stop_flag.store(false, std::memory_order_release);

  struct sigaction action {};
  action.sa_handler = OnShutdownSignal;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(SIGINT, &action, nullptr) != 0) {
    spdlog::error("[app_runner] failed to install SIGINT handler: {}", std::strerror(errno));
  }
  if (sigaction(SIGTERM, &action, nullptr) != 0) {
    spdlog::error("[app_runner] failed to install SIGTERM handler: {}", std::strerror(errno));
  }
}

void BlockShutdownSignals() {
  ApplySignalMask(SIG_BLOCK);
}

void UnblockShutdownSignals() {
  ApplySignalMask(SIG_UNBLOCK);
}

void RunUntilStop(std::atomic<bool>& stop_flag) {
  while (!g_stop_requested.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  stop_flag.store(true, std::memory_order_release);
  spdlog::info("[app_runner] shutdown signal received, exiting...");
}

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

int RunServiceMain(int argc, char** argv, const std::string& service_name, const std::string& log_filename) {
  std::string config_path;
  if (!ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[" << service_name << "] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  if (!init_spdlog_logger("logs", log_filename)) {
    return EXIT_FAILURE;
  }

  spdlog::info("==================================================");
  spdlog::info("{} starting (pid={})", service_name, getpid());
  spdlog::info("config: {}", config_path);
  spdlog::info("==================================================");
  spdlog::info("[{}] service loop active (stub)", service_name);

  std::atomic<bool> stop_flag{false};
  InstallShutdownHandler(stop_flag);
  UnblockShutdownSignals();
  spdlog::info("[{}] waiting for SIGINT/SIGTERM...", service_name);

  RunUntilStop(stop_flag);

  spdlog::info("[{}] stopped cleanly", service_name);
  return EXIT_SUCCESS;
}

}  // namespace qtrade::common
