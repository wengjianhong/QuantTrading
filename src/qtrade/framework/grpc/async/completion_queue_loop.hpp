/// @file      completion_queue_loop.hpp
/// @brief     gRPC ServerCompletionQueue 轮询线程
/// @details   在独立线程中 Next 事件并分发到 CallTagBase::Proceed；不负责 Shutdown CQ
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_COMPLETION_QUEUE_LOOP_HPP_
#define QTRADE_COMMON_GRPC_COMPLETION_QUEUE_LOOP_HPP_

#include <grpcpp/completion_queue.h>

#include <cstddef>
#include <thread>
#include <vector>

namespace qtrade::common::grpc_async {

/// @brief 在独立线程中轮询 CQ 并分发 CallTagBase::Proceed
class CompletionQueueLoop {
 public:
  /// @brief 构造未启动的轮询器
  CompletionQueueLoop() = default;

  /// @brief 析构时 Stop，等待轮询线程退出
  ~CompletionQueueLoop();

  CompletionQueueLoop(const CompletionQueueLoop&) = delete;
  CompletionQueueLoop& operator=(const CompletionQueueLoop&) = delete;

  /// @brief 启动轮询线程
  /// @param cq 服务端 CompletionQueue（生命周期须覆盖本对象；不可为 nullptr）
  /// @param thread_count 轮询线程数；为 0 时按 1 处理
  void Start(grpc::ServerCompletionQueue* cq, std::size_t thread_count = 1);

  /// @brief 停止轮询并 join 线程（不 Shutdown CQ，由 GrpcAsyncServer 负责）
  void Stop();

  /// @brief 查询轮询是否已启动
  /// @return Start 成功且尚未 Stop 时为 true
  [[nodiscard]] bool IsRunning() const {
    return running_;
  }

 private:
  /// @brief 单线程轮询循环：Next → CallTagBase::Proceed
  void Run();

  /// 是否正在轮询
  bool running_ = false;
  /// 轮询线程集合
  std::vector<std::thread> threads_;
  /// 非拥有的 CQ 指针
  grpc::ServerCompletionQueue* cq_ = nullptr;
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_COMPLETION_QUEUE_LOOP_HPP_
