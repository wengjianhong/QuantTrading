/// @file      grpc_async_server.hpp
/// @brief     gRPC Async 服务端封装（Server + CQ + 轮询线程）
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_GRPC_ASYNC_SERVER_HPP_
#define QTRADE_COMMON_GRPC_GRPC_ASYNC_SERVER_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace grpc {
class Server;
class ServerCompletionQueue;
class Service;
}  // namespace grpc

namespace qtrade::common::grpc_async {

class CompletionQueueLoop;

/// @brief 支撑服务侧 gRPC Async + CQ 运行时
class GrpcAsyncServer {
 public:
  struct Options {
    std::string listen_address;       ///< 监听地址
    std::size_t cq_thread_count = 1;  ///< CQ 轮询线程数
  };

  GrpcAsyncServer();
  ~GrpcAsyncServer();

  GrpcAsyncServer(const GrpcAsyncServer&) = delete;
  GrpcAsyncServer& operator=(const GrpcAsyncServer&) = delete;

  /// @brief 注册 AsyncService 并启动监听与 CQ 轮询
  /// @param options 监听与线程配置
  /// @param async_service 须为 xxx::AsyncService 实例
  ErrorCode Start(const Options& options, grpc::Service* async_service);

  /// @brief 优雅停止：Shutdown Server → Shutdown CQ → 停止轮询线程
  void Shutdown();

  /// @brief 阻塞直至 Server 完全停止
  void Wait();

  [[nodiscard]] grpc::ServerCompletionQueue* CompletionQueue() const { return cq_.get(); }
  [[nodiscard]] bool IsRunning() const { return running_; }

 private:
  std::unique_ptr<grpc::Server> server_;
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<CompletionQueueLoop> loop_;
  bool running_ = false;
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_GRPC_ASYNC_SERVER_HPP_
