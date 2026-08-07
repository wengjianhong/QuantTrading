/// @file      process_boot.cpp
/// @brief     进程启动共用原语实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/boot/process_boot.hpp"

#include "qtrade/common/logging/logger.hpp"

#include <spdlog/spdlog.h>

#include <iostream>
#include <unistd.h>

namespace qtrade::common::process_boot {

Result<ProgramOptions> ParseProgramOptions(int argc, char** argv) {
  ProgramOptions options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      options.config_path = argv[++i];
    }
  }

  return Result<ProgramOptions>{ErrorCode::kSuccess, "success", options};
}

bool InitProgramEnvironment(const std::string& service_name,
                            const std::string& log_dir,
                            const std::string& log_filename,
                            const ProgramOptions& options) {
  if (!InitSpdlogLogger(log_dir, log_filename)) {
    std::cerr << "failed to initialize logger, service_name=" << service_name << std::endl;
    return false;
  }

  spdlog::info("[process_boot] InitProgramEnvironment");
  spdlog::info("service_name={}", service_name);
  spdlog::info("config_path={}", options.config_path);
  spdlog::info("service_pid={}", getpid());
  return true;
}

void LogProcessStopped(const std::string& service_name) {
  spdlog::info("service_pid={}", getpid());
  spdlog::info("service_name={}", service_name);
  spdlog::info("[{}] stopped cleanly", service_name);
}

}  // namespace qtrade::common::process_boot
