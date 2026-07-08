/// @file      support_service_main.hpp
/// @brief     支撑服务独立进程通用入口
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_APP_SUPPORT_SERVICE_MAIN_HPP_
#define QTRADE_COMMON_APP_SUPPORT_SERVICE_MAIN_HPP_

#include "common/app/app_runner.hpp"
#include "common/logging/logger.hpp"
#include "common/support/support_service.hpp"

#include <qtrade/error_code/code_message.hpp>
#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>

namespace qtrade::common {

/// @brief 支撑服务独立进程通用入口
/// @param config_path 已在 main 中解析的配置文件路径
/// @param log_dir 日志目录
/// @param log_filename 日志文件名
/// @param service 支撑服务实例
inline int RunSupportServiceMain(const std::string& config_path,
                                 const std::string& log_dir,
                                 const std::string& log_filename,
                                 support::ISupportService& service) {
  if (!init_spdlog_logger(log_dir, log_filename)) {
    return EXIT_FAILURE;
  }

  const std::string_view service_name = service.GetStatus().service_name;
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

  spdlog::info("[{}] stopped cleanly", service_name);
  return EXIT_SUCCESS;
}

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_APP_SUPPORT_SERVICE_MAIN_HPP_
