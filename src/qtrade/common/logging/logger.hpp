/// @file      logger.hpp
/// @brief     日志模块
/// @details   基于 spdlog 的高性能全局日志系统
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_LOGGING_LOGGER_HPP_
#define QTRADE_COMMON_LOGGING_LOGGER_HPP_

#include <spdlog/spdlog.h>

#include <cstddef>
#include <string>

namespace qtrade::common {

/// @brief   初始化 spdlog 日志
/// @param log_dir      日志目录
/// @param log_filename 日志文件名
/// @param max_size_mb  单个文件最大大小(MB)
/// @param max_files    最大保留文件数
/// @param level        日志级别
bool init_spdlog_logger(const std::string& log_dir = "logs",
                        const std::string& log_filename = "qtrade.log",
                        std::size_t max_size_mb = 50,
                        std::size_t max_files = 10,
                        spdlog::level::level_enum level = spdlog::level::info);

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_LOGGING_LOGGER_HPP_
