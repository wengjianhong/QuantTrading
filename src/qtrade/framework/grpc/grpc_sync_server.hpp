/// @file      grpc_sync_server.hpp
/// @brief     gRPC 同步服务端封装（ServerBuilder + 线程池）
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_GRPC_SYNC_SERVER_HPP_
#define QTRADE_COMMON_GRPC_GRPC_SYNC_SERVER_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>

namespace grpc {
class Server;
class Service;
}  // namespace grpc

namespace qtrade::common::grpc_sync {

/// @brief 支撑服务侧 gRPC 同步运行时
class GrpcSyncServer {
 public:
  GrpcSyncServer();
  ~GrpcSyncServer();

  GrpcSyncServer(const GrpcSyncServer&) = delete;
  GrpcSyncServer& operator=(const GrpcSyncServer&) = delete;

  /// @brief 注册同步 Service 并启动监听
  ErrorCode Start(const std::string& listen_address, grpc::Service* sync_service);

  /// @brief 优雅停止
  void Shutdown();

  /// @brief 阻塞直至 Server 完全停止
  void Wait();

  [[nodiscard]] bool IsRunning() const {
    return running_;
  }

 private:
  std::unique_ptr<grpc::Server> server_;
  bool running_ = false;
};

}  // namespace qtrade::common::grpc_sync

#endif  // QTRADE_COMMON_GRPC_GRPC_SYNC_SERVER_HPP_
