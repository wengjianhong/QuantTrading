/// @file      grpc_sync_server.hpp
/// @brief     gRPC 同步服务端封装（ServerBuilder）
/// @details   纯传输层：负责监听与启停，不包含业务 Service 实现
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_GRPC_SYNC_SERVER_HPP_
#define QTRADE_COMMON_GRPC_GRPC_SYNC_SERVER_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

namespace qtrade::common::grpc_sync {

/// @brief 支撑服务侧 gRPC 同步运行时
class GrpcSyncServer {
 public:
  /// @brief 构造未启动的同步 Server
  GrpcSyncServer();

  /// @brief 析构时 Shutdown 并 Wait，确保资源释放
  ~GrpcSyncServer();

  GrpcSyncServer(const GrpcSyncServer&) = delete;
  GrpcSyncServer& operator=(const GrpcSyncServer&) = delete;

  /// @brief 注册同步 Service 并启动监听
  /// @param listen_address 监听地址，如 0.0.0.0:50052
  /// @param sync_service protobuf 生成的同步 Service 实例（不可为 nullptr；生命周期须覆盖运行期）
  /// @return 成功返回 ErrorCode::kSuccess；已在运行、参数非法或 BuildAndStart 失败返回错误码
  ErrorCode Start(const std::string& listen_address, grpc::Service* sync_service);

  /// @brief 优雅停止监听（不阻塞 Wait）
  void Shutdown();

  /// @brief 阻塞直至 Server 完全停止并释放底层对象
  void Wait();

  /// @brief 查询是否处于运行中
  /// @return 已 Start 且尚未 Shutdown 时为 true
  [[nodiscard]] bool IsRunning() const {
    return running_;
  }

 private:
  /// gRPC Server 实例
  std::unique_ptr<grpc::Server> server_;
  /// 是否正在运行
  bool running_ = false;
};

}  // namespace qtrade::common::grpc_sync

#endif  // QTRADE_COMMON_GRPC_GRPC_SYNC_SERVER_HPP_
