/// @file      process_boot.hpp
/// @brief     进程启动共用原语（引擎与支撑服务复用）
/// @details   命令行解析、日志横幅、等待停机信号、停机日志。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_APP_PROCESS_BOOT_HPP_
#define QTRADE_COMMON_APP_PROCESS_BOOT_HPP_

#include <string>

namespace qtrade::common::process_boot {

/// @brief 解析命令行中的 --config 参数
/// @return 找到并成功解析 --config <path> 时返回 true
[[nodiscard]] bool ParseConfigPath(int argc, char** argv, std::string& config_path);

/// @brief 初始化日志并打印进程启动横幅
[[nodiscard]] bool InitProgramEnv(const std::string& service_name,
                                  const std::string& log_dir,
                                  const std::string& log_filename,
                                  const std::string& config_path);

/// @brief 阻塞等待 SIGINT/SIGTERM，并 NotifyStopping
/// @return 收到的信号编号；失败返回 -1
[[nodiscard]] int WaitForInterruptAndNotifyStopping(const std::string& service_name);

/// @brief 打印进程停止横幅
/// @param service_name 服务名称，用于标识日志来源
void LogProcessStopped(const std::string& service_name);

}  // namespace qtrade::common::process_boot

#endif  // QTRADE_COMMON_APP_PROCESS_BOOT_HPP_
