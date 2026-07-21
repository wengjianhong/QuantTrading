/// @file      process_boot.hpp
/// @brief     进程启动共用原语（引擎与支撑服务复用）
/// @details   命令行解析、日志横幅、等待停机信号、停机日志。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_APP_PROCESS_BOOT_HPP_
#define QTRADE_COMMON_APP_PROCESS_BOOT_HPP_

#include <qtrade/structs/result.hpp>

#include <string>

namespace qtrade::common::process_boot {

/// @brief 程序命令行选项
struct ProgramOptions {
  /// @brief 配置文件路径
  std::string config_path;
};

/// @brief 解析命令行中的 --config 参数
/// @return 找到并成功解析 --config <path> 时返回 true
[[nodiscard]] Result<ProgramOptions> ParseProgramOptions(int argc, char** argv);

/// @brief 初始化服务全局环境（日志）
/// @param service_name 服务名称，用于标识日志来源
/// @param log_dir 日志目录
/// @param log_filename 日志文件名
/// @param options 程序命令行选项
/// @return 成功返回 true，失败返回 false
[[nodiscard]] bool InitProgramEnvironment(const std::string& service_name,
                                          const std::string& log_dir,
                                          const std::string& log_filename,
                                          const ProgramOptions& options);

/// @brief 打印进程停止信息
/// @param service_name 服务名称
void LogProcessStopped(const std::string& service_name);

}  // namespace qtrade::common::process_boot

#endif  // QTRADE_COMMON_APP_PROCESS_BOOT_HPP_
