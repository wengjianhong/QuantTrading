/// @file      support_service_boot.hpp
/// @brief     支撑服务进程启动阶段（业务相关）
/// @details   共用阶段见 process_boot；本文件仅 ISupportService 生命周期编排。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_APP_SUPPORT_BOOT_HPP_
#define QTRADE_COMMON_APP_SUPPORT_BOOT_HPP_

#include "qtrade/common/boot/process_boot.hpp"

#include <qtrade/support/support_service.hpp>

#include <string>

namespace qtrade::common::support_boot {

/// @brief 使用配置文件调用 service.Initialize
[[nodiscard]] bool InitializeService(support::ISupportService& service, const std::string& config_path);

/// @brief 调用 service.Start
[[nodiscard]] bool StartService(support::ISupportService& service);

/// @brief 阻塞至停机信号后 Stop 并等待 Wait 线程退出
void RunUntilShutdown(support::ISupportService& service);

/// @brief 支撑服务独立进程入口
/// @details block 信号 → InitProgramEnvironment → InitializeService → StartService → RunUntilShutdown
///
/// @param options 程序选项
/// @param log_dir 日志目录
/// @param log_filename 日志文件名
/// @param service 支撑服务实例
/// @return 正常退出返回 EXIT_SUCCESS，初始化或启动失败返回 EXIT_FAILURE
int RunSupportServiceMain(const process_boot::ProgramOptions& options,
                          const std::string& log_dir,
                          const std::string& log_filename,
                          support::ISupportService& service);

}  // namespace qtrade::common::support_boot

#endif  // QTRADE_COMMON_APP_SUPPORT_BOOT_HPP_
