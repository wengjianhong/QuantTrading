/// @file      app_runner.hpp
/// @brief     独立进程通用启动与优雅退出
/// @details   提供命令行参数解析、SIGINT/SIGTERM 优雅退出及多线程信号屏蔽能力
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_APP_APP_RUNNER_HPP_
#define QTRADE_COMMON_APP_APP_RUNNER_HPP_

#include <atomic>
#include <string>

namespace qtrade::common {

/// @brief 安装进程级 SIGINT/SIGTERM 关闭处理器
/// @details 使用 sigaction 注册 handler；handler 内仅写内部 g_stop_requested（async-signal-safe），
///          不执行日志或资源释放。进程内只需调用一次，且应在 worker 线程启动前完成。
/// @param stop_flag 调用方退出标志；安装时清零，RunUntilStop 检测到信号后同步为 true
void InstallShutdownHandler(std::atomic<bool>& stop_flag);

/// @brief 在当前线程屏蔽 SIGINT/SIGTERM
/// @details 对调用线程执行 pthread_sigmask(SIG_BLOCK, ...)。被屏蔽的信号不会投递到本线程，
///          亦不会在本线程上下文中执行 handler。worker / 热路径线程应在入口第一行调用。
void BlockShutdownSignals();

/// @brief 在当前线程恢复接收 SIGINT/SIGTERM
/// @details 对调用线程执行 pthread_sigmask(SIG_UNBLOCK, ...)。仅表示本线程可被投递信号，
///          不注册 handler；须配合 InstallShutdownHandler 与 RunUntilStop 使用。
///          主线程或专用控制线程在 RunUntilStop 之前调用。
void UnblockShutdownSignals();

/// @brief 阻塞直至收到退出信号
/// @details 在普通线程上下文中轮询内部 g_stop_requested（100ms 间隔），适用于主线程等待退出信号。
///          检测到信号后将 stop_flag 置为 true 并返回，调用方可在返回后安全执行 engine.Stop() 等清理逻辑。
/// @param stop_flag 调用方退出标志；返回前会被置为 true
void RunUntilStop(std::atomic<bool>& stop_flag);

/// @brief 解析命令行中的 --config 参数
/// @param argc      main 传入的参数个数
/// @param argv      main 传入的参数数组
/// @param config_path 输出：解析到的 JSON 配置文件路径；未找到 --config 时不修改
/// @return 找到并成功解析 --config <path> 时返回 true，否则返回 false
bool ParseConfigPath(int argc, char** argv, std::string& config_path);

/// @brief 支撑服务独立进程通用入口（stub 主循环）
/// @details 解析 --config、初始化日志、安装信号处理并阻塞至 SIGINT/SIGTERM。
///          后续可在此扩展 Kafka/gRPC 服务端 bootstrap。
/// @param argc         main 传入的参数个数
/// @param argv         main 传入的参数数组
/// @param service_name 服务名，用于日志标识
/// @param log_filename 日志文件名（写入 logs/ 目录）
/// @return 正常退出返回 EXIT_SUCCESS，参数或初始化失败返回 EXIT_FAILURE
int RunServiceMain(int argc, char** argv, const std::string& service_name, const std::string& log_filename);

}  // namespace qtrade::common

#endif  // QTRADE_COMMON_APP_APP_RUNNER_HPP_
