/// @file      grpc_async_server.hpp
/// @brief     gRPC Async 服务端封装（Server + CQ + 轮询线程）
/// @details   纯传输层：负责监听、CQ 与轮询启停；CallTag / 业务 Handler 由上层装配
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_GRPC_ASYNC_SERVER_HPP_
#define QTRADE_COMMON_GRPC_GRPC_ASYNC_SERVER_HPP_

#include "qtrade/framework/grpc/async/completion_queue_loop.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <grpcpp/grpcpp.h>

#include <cstddef>
#include <memory>
#include <string>

namespace qtrade::common::grpc_async {

/// @brief 支撑服务侧 gRPC Async + CQ 运行时
class GrpcAsyncServer {
 public:
  /// @brief Async Server 启动选项
  struct Options {
    /// 监听地址，如 0.0.0.0:50051
    std::string listen_address;
    /// CQ 轮询线程数
    std::size_t cq_thread_count = 1;
  };

  /// @brief 构造未启动的异步 Server
  GrpcAsyncServer();

  /// @brief 析构时 Shutdown 并 Wait，确保 CQ 与线程退出
  ~GrpcAsyncServer();

  GrpcAsyncServer(const GrpcAsyncServer&) = delete;
  GrpcAsyncServer& operator=(const GrpcAsyncServer&) = delete;

  /// @brief 注册 AsyncService，启动监听与 CQ 轮询
  /// @param options 监听地址与 CQ 线程数
  /// @param async_service protobuf xxx::AsyncService 实例（不可为 nullptr；生命周期须覆盖运行期）
  /// @return 成功返回 ErrorCode::kSuccess；已在运行、参数非法或启动失败返回错误码
  ErrorCode Start(const Options& options, grpc::Service* async_service);

  /// @brief 优雅停止：Shutdown Server → Shutdown CQ → 停止轮询线程
  void Shutdown();

  /// @brief 阻塞直至 Server 完全停止并释放底层对象
  void Wait();

  /// @brief 获取服务端 CompletionQueue（供 Handler 注册 CallTag）
  /// @return CQ 指针；未启动时可能为空
  [[nodiscard]] grpc::ServerCompletionQueue* CompletionQueue() const {
    return cq_.get();
  }

  /// @brief 查询是否处于运行中
  /// @return 已 Start 且尚未 Shutdown 时为 true
  [[nodiscard]] bool IsRunning() const {
    return running_;
  }

 private:
  /// 是否正在运行
  bool running_ = false;
  /// gRPC Server 实例
  std::unique_ptr<grpc::Server> server_;
  /// CQ 轮询线程
  std::unique_ptr<CompletionQueueLoop> loop_;
  /// 服务端 CompletionQueue
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_GRPC_ASYNC_SERVER_HPP_
