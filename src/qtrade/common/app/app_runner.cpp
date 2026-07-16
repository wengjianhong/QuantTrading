/// @file      app_runner.cpp
/// @brief     应用启动器实现
/// @details   实现信号处理、配置路径解析及服务进程通用主循环
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/app_runner.hpp"

#include "qtrade/common/logging/logger.hpp"

#include <qtrade/error_code/code_message.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_framework/support/support_service.hpp>

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

int RunSupportServiceMain(const std::string& config_path,
                          const std::string& log_dir,
                          const std::string& log_filename,
                          support::ISupportService& service) {
  const std::string service_name = service.GetStatus().service_name;
  if (!init_spdlog_logger(log_dir, log_filename)) {
    std::cerr << "failed to initialize logger, service_name=" << service_name << std::endl;
    return EXIT_FAILURE;
  }

  spdlog::info("======================= Starting =========================");
  spdlog::info("service_name={}", service_name);
  spdlog::info("config_path={}", config_path);
  spdlog::info("service_pid={}", getpid());

  if (const ErrorCode error_code = service.Initialize(config_path); error_code != ErrorCode::kSuccess) {
    spdlog::error("[{}] initialize failed: {}", service_name, qtrade::GetErrorCodeMessage(error_code));
    return EXIT_FAILURE;
  }

  if (const ErrorCode error_code = service.Start(); error_code != ErrorCode::kSuccess) {
    spdlog::error("[{}] start failed: {}", service_name, qtrade::GetErrorCodeMessage(error_code));
    service.Stop();
    return EXIT_FAILURE;
  }

  std::atomic<bool> stop_flag{false};
  InstallShutdownHandler(stop_flag);
  std::thread service_thread([&service] { service.Wait(); });

  UnblockShutdownSignals();
  spdlog::info("[{}] running until SIGINT/SIGTERM...", service_name);
  RunUntilStop(stop_flag);

  service.Stop();
  if (service_thread.joinable()) {
    service_thread.join();
  }

  spdlog::info("service_pid={}", getpid());
  spdlog::info("service_name={}", service_name);
  spdlog::info("======================= Stopped =========================");
  spdlog::info("[{}] stopped cleanly", service_name);
  return EXIT_SUCCESS;
}

}  // namespace qtrade::common
