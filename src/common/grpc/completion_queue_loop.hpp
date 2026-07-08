/// @file      completion_queue_loop.hpp
/// @brief     gRPC ServerCompletionQueue 轮询线程
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_COMPLETION_QUEUE_LOOP_HPP_
#define QTRADE_COMMON_GRPC_COMPLETION_QUEUE_LOOP_HPP_

#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace grpc {
class ServerCompletionQueue;
}  // namespace grpc

namespace qtrade::common::grpc_async {

/// @brief 在独立线程中 AsyncNext/Next 分发 CallTagBase::Proceed
class CompletionQueueLoop {
 public:
  CompletionQueueLoop() = default;
  ~CompletionQueueLoop();

  CompletionQueueLoop(const CompletionQueueLoop&) = delete;
  CompletionQueueLoop& operator=(const CompletionQueueLoop&) = delete;

  /// @brief 启动轮询线程
  /// @param cq 服务端 CompletionQueue（生命周期须覆盖本对象）
  /// @param thread_count 轮询线程数，默认 1
  void Start(grpc::ServerCompletionQueue* cq, std::size_t thread_count = 1);

  /// @brief 停止轮询（不 Shutdown CQ，由 GrpcAsyncServer 负责）
  void Stop();

  [[nodiscard]] bool IsRunning() const { return running_; }

 private:
  void Run();

  grpc::ServerCompletionQueue* cq_ = nullptr;
  std::vector<std::thread> threads_;
  bool running_ = false;
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_COMPLETION_QUEUE_LOOP_HPP_
