/// @file      config_server.hpp
/// @brief     config-service gRPC 进程封装
/// @author    wengjianhong
/// @date      2026-06-22
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_SERVER_HPP_
#define QTRADE_SERVICE_CONFIG_SERVER_HPP_

#include "service/config_service/repository/config_repository.hpp"

#include <qtrade/config/v1/config.grpc.pb.h>
#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>

namespace qtrade::common::grpc_async {
class GrpcAsyncServer;
}

namespace qtrade::service {

class ConfigGrpcAsyncHandler;

/// @brief config-service 启动上下文
struct ConfigServiceContext {
  std::shared_ptr<IConfigRepository> repository;  ///< 数据库仓储；database.enabled=false 时为 nullptr
};

/// @brief 启动/停止 config-service gRPC 监听（Async + CQ）
class ConfigServer {
 public:
  /// @brief 构造 ConfigServer 实例
  ConfigServer();

  /// @brief 析构并调用 Shutdown
  ~ConfigServer();

  ConfigServer(const ConfigServer&) = delete;
  ConfigServer& operator=(const ConfigServer&) = delete;

  /// @brief 启动 gRPC 监听
  /// @param listen_address 监听地址，如 0.0.0.0:50051
  /// @param context 数据库仓储上下文
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Start(const std::string& listen_address, const ConfigServiceContext& context);

  /// @brief 优雅停止 gRPC 服务
  void Shutdown();

  /// @brief 阻塞直至服务停止
  void Wait();

  /// @brief 服务是否正在运行
  /// @return true 表示已启动且未 Shutdown
  [[nodiscard]] bool IsRunning() const { return running_; }

 private:
  qtrade::config::v1::ConfigService::AsyncService async_service_;             ///< gRPC 异步服务实例
  std::unique_ptr<qtrade::common::grpc_async::GrpcAsyncServer> grpc_server_;  ///< gRPC 服务器
  std::unique_ptr<ConfigGrpcAsyncHandler> handler_;                           ///< RPC 处理器
  std::shared_ptr<IConfigRepository> repository_;                             ///< 数据库仓储
  bool running_ = false;                                                      ///< 是否正在运行
};

/// @brief 启动引导：连接数据库并确保表结构
/// @param json_path 服务配置文件路径（qtrade_config_service.json）
/// @return 启动上下文；database 未启用或连接失败时 repository 为 nullptr
[[nodiscard]] ConfigServiceContext BootstrapConfigService(const std::string& json_path);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_SERVER_HPP_
