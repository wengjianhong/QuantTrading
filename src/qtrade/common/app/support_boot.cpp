/// @file      support_boot.cpp
/// @brief     支撑服务进程启动阶段实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/support_boot.hpp"

#include "qtrade/common/app/process_boot.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/common/system/systemd_notify.hpp"

#include <qtrade/error_code/code_message.hpp>
#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <thread>

namespace qtrade::common::support_boot {

bool InitializeService(support::ISupportService& service, const std::string& config_path) {
  const std::string service_name = service.GetStatus().service_name;
  spdlog::info("[support_boot] InitializeService");
  if (const ErrorCode error_code = service.Initialize(config_path); error_code != ErrorCode::kSuccess) {
    spdlog::error("[{}] initialize failed: {}", service_name, qtrade::GetErrorCodeMessage(error_code));
    return false;
  }
  return true;
}

bool StartService(support::ISupportService& service) {
  const std::string service_name = service.GetStatus().service_name;
  spdlog::info("[support_boot] StartService");
  if (const ErrorCode error_code = service.Start(); error_code != ErrorCode::kSuccess) {
    spdlog::error("[{}] start failed: {}", service_name, qtrade::GetErrorCodeMessage(error_code));
    service.Stop();
    return false;
  }
  return true;
}

void RunUntilShutdown(support::ISupportService& service) {
  const std::string service_name = service.GetStatus().service_name;
  spdlog::info("[support_boot] RunUntilShutdown");

  std::thread service_thread([&service] { service.Wait(); });
  (void)process_boot::WaitForInterruptAndNotifyStopping(service_name);

  service.Stop();
  if (service_thread.joinable()) {
    service_thread.join();
  }
  process_boot::LogProcessStopped(service_name);
}

int RunSupportServiceMain(const std::string& config_path,
                          const std::string& log_dir,
                          const std::string& log_filename,
                          support::ISupportService& service) {
  const std::string service_name = service.GetStatus().service_name;

  // 1. 尽早阻塞 SIGINT/SIGTERM
  system::BlockInterruptSignals();

  // 2. 初始化程序全局环境（日志、启动横幅）
  if (!process_boot::InitProgramEnv(service_name, log_dir, log_filename, config_path)) {
    system::NotifyError(0, "Failed to initialize program environment");
    return EXIT_FAILURE;
  }

  // 3. 初始化服务（读配置、建依赖）
  if (!InitializeService(service, config_path)) {
    system::NotifyError(0, "Failed to initialize service");
    return EXIT_FAILURE;
  }

  // 4. 启动对外服务（如 gRPC 监听）
  if (!StartService(service)) {
    system::NotifyError(0, "Failed to start service");
    return EXIT_FAILURE;
  }

  // 5. 通知 systemd 服务就绪
  (void)system::NotifyReady(service_name + " ready");

  // 6. 阻塞运行直至停机信号，并释放服务资源
  RunUntilShutdown(service);
  return EXIT_SUCCESS;
}

}  // namespace qtrade::common::support_boot
