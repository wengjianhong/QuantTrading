/// @file      logger.cpp
/// @brief     日志模块实现
/// @details   基于 spdlog 初始化全局日志器，支持轮转文件输出
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/logging/logger.hpp"

#include <spdlog/sinks/rotating_file_sink.h>

#include <cstdio>

namespace qtrade::common {

bool InitSpdlogLogger(const std::string& log_dir,
                      const std::string& log_filename,
                      std::size_t max_size_mb,
                      std::size_t max_files,
                      spdlog::level::level_enum level) {
  try {
    const std::size_t rotate_size = max_size_mb * 1024 * 1024;

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      log_dir + "/" + log_filename, rotate_size, max_files, false);

#ifdef DEBUG
    file_sink->set_pattern("[%Y-%m-%d %X.%e] [%l] [%P] [%t] [%g:%#] %v");
#else
    file_sink->set_pattern("[%Y-%m-%d %X.%e] [%l] [%P] [%t] %v");
#endif
    file_sink->set_level(spdlog::level::trace);

    auto logger = std::make_shared<spdlog::logger>("global", file_sink);
    logger->set_level(level);
    logger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(logger);

    spdlog::info("[logger] init success (file-only mode)");
    spdlog::info("[logger] log path: {}/{}", log_dir, log_filename);
    spdlog::info("[logger] max size: {}MB, max files: {}", max_size_mb, max_files);
  } catch (const spdlog::spdlog_ex& ex) {
    fprintf(stderr, "spdlog init failed: %s\n", ex.what());
    return false;
  }

  return true;
}

}  // namespace qtrade::common
