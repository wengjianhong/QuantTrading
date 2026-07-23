/// @file      support_boot.cpp
/// @brief     支撑服务进程启动阶段实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/boot/support_service_boot.hpp"

#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/common/system/systemd_notify.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <thread>

namespace qtrade::common::support_boot {

bool InitializeService(support::ISupportService& service, const std::string& config_path) {
  const std::string service_name = service.GetStatus().service_name;
  spdlog::info("[support_boot] InitializeService service_name={}", service_name);

  const ErrorCode error_code = service.Initialize(config_path);
  if (error_code != ErrorCode::kSuccess) {
    spdlog::error("[{}] initialize failed: {}", service_name, qtrade::GetErrorCodeMessage(error_code));
    return false;
  }
  return true;
}

bool StartService(support::ISupportService& service) {
  const std::string service_name = service.GetStatus().service_name;
  spdlog::info("[support_boot] StartService service_name={}", service_name);

  const ErrorCode error_code = service.Start();
  if (error_code != ErrorCode::kSuccess) {
    spdlog::error("[{}] start failed: {}", service_name, qtrade::GetErrorCodeMessage(error_code));
    service.Stop();
    return false;
  }
  return true;
}

void RunUntilShutdown(support::ISupportService& service) {
  const std::string service_name = service.GetStatus().service_name;
  spdlog::info("[support_boot] RunUntilShutdown service_name={}", service_name);

  std::thread service_thread([&service] { service.Wait(); });
  spdlog::info("[{}] running until SIGINT/SIGTERM...", service_name);

  const int signal = qtrade::common::system::WaitInterruptSignals();
  spdlog::info("[{}] received signal {}, stopping...", service_name, signal);

  service.Stop();
  if (service_thread.joinable()) {
    service_thread.join();
  }
}

int RunSupportServiceMain(const process_boot::ProgramOptions& options,
                          const std::string& log_dir,
                          const std::string& log_filename,
                          support::ISupportService& service) {
  const std::string service_name = service.GetStatus().service_name;

  // 1. 阻塞 SIGINT/SIGTERM，避免信号打到子线程导致直接退出
  system::BlockInterruptSignals();

  // 2. 初始化程序全局环境（日志、启动横幅）
  if (!process_boot::InitProgramEnvironment(service_name, log_dir, log_filename, options)) {
    system::NotifyError(0, "Failed to initialize program environment");
    return EXIT_FAILURE;
  }

  // 3. 初始化服务（读配置、建依赖）
  if (!InitializeService(service, options.config_path)) {
    system::NotifyError(0, "Failed to initialize service");
    return EXIT_FAILURE;
  }

  // 4. 启动对外服务（如 gRPC 监听）
  if (!StartService(service)) {
    system::NotifyError(0, "Failed to start service");
    return EXIT_FAILURE;
  }
  (void)system::NotifyReady(service_name + " ready");

  // 5. 阻塞运行直至停机信号，并释放服务资源
  RunUntilShutdown(service);

  // 6. 打印进程停止信息
  process_boot::LogProcessStopped(service_name);
  return EXIT_SUCCESS;
}

}  // namespace qtrade::common::support_boot
