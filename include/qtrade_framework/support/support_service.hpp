/// @file      support_service.hpp
/// @brief     支撑服务进程生命周期接口（与 gRPC Service 无关）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_FRAMEWORK_SUPPORT_SUPPORT_SERVICE_HPP_
#define QTRADE_FRAMEWORK_SUPPORT_SUPPORT_SERVICE_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <string>

namespace qtrade::common::support {

/// @brief 支撑服务进程生命周期状态
/// @details 参考进程状态模型，状态流转：New → Initializing → Ready → Stopping → Terminated；
///          Initialize 失败或 Start 失败进入 Failed。
///          Wait() 线程阻塞，不单独暴露为状态。
enum class SupportServiceState {
  kNew = 0,           ///< 新建状态：已构造，尚未调用 Initialize
  kInitializing = 1,  ///< 初始状态：Initialize 进行中或依赖已就绪，尚未 Start
  kReady = 2,         ///< 就绪状态：对外服务已启动，可接受请求
  kStopping = 3,      ///< 终止状态：正在优雅关闭
  kTerminated = 4,    ///< 已终止状态：资源已释放，可再次 Initialize
  kFailed = 5,        ///< 失败状态：Initialize 或 Start 未成功
};

/// @brief 支撑服务进程运行状态快照
struct SupportServiceStatus {
  std::string service_name;                               ///< 服务名
  std::string config_path;                                ///< 配置文件路径
  std::string listen_address;                             ///< gRPC 监听地址（若适用）
  std::string last_error_message;                         ///< 最近一次失败错误信息
  ErrorCode last_error = ErrorCode::kSuccess;             ///< 最近一次失败错误码
  SupportServiceState state = SupportServiceState::kNew;  ///< 生命周期状态
};

/// @brief 支撑服务进程统一生命周期接口
/// @details 每个 qtrade_*_service 进程实现此接口，负责启停、状态查询等；与 protobuf/gRPC Service 无关。
class ISupportService {
 public:
  virtual ~ISupportService() = default;

  /// @brief 读取配置并初始化依赖（数据库、仓储等）
  virtual ErrorCode Initialize(const std::string& config_path) = 0;

  /// @brief 启动对外服务（如 gRPC 监听）
  virtual ErrorCode Start() = 0;

  /// @brief 优雅停止服务（不阻塞等待后台线程退出）
  virtual void Stop() = 0;

  /// @brief 阻塞直至后台工作线程退出（须在独立线程调用）
  virtual void Wait() = 0;

  /// @brief 获取当前运行状态
  [[nodiscard]] virtual SupportServiceStatus GetStatus() const = 0;
};

}  // namespace qtrade::common::support

#endif  // QTRADE_FRAMEWORK_SUPPORT_SUPPORT_SERVICE_HPP_
