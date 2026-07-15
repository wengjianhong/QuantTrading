/// @file      completion_queue_loop.cpp
/// @brief     CompletionQueueLoop 实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/grpc/async/completion_queue_loop.hpp"

#include "qtrade/framework/grpc/async/call_tag_base.hpp"

#include <grpcpp/completion_queue.h>
#include <spdlog/spdlog.h>

namespace qtrade::common::grpc_async {

CompletionQueueLoop::~CompletionQueueLoop() {
  Stop();
}

void CompletionQueueLoop::Start(grpc::ServerCompletionQueue* cq, std::size_t thread_count) {
  // 1. 校验状态与参数
  if (running_ || cq == nullptr) {
    return;
  }
  if (thread_count == 0) {
    thread_count = 1;
  }

  // 2. 启动轮询线程
  cq_ = cq;
  running_ = true;
  threads_.reserve(thread_count);
  for (std::size_t i = 0; i < thread_count; ++i) {
    threads_.emplace_back([this] { Run(); });
  }
  spdlog::info("[CompletionQueueLoop] started {} poller thread(s)", thread_count);
}

void CompletionQueueLoop::Stop() {
  if (!running_) {
    return;
  }
  running_ = false;
  for (auto& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  threads_.clear();
  cq_ = nullptr;
}

void CompletionQueueLoop::Run() {
  void* tag = nullptr;
  bool ok = false;
  while (cq_->Next(&tag, &ok)) {
    if (tag == nullptr) {
      continue;
    }
    auto* tag_obj = static_cast<CallTagBase*>(tag);
    tag_obj->Proceed(ok);
  }
}

}  // namespace qtrade::common::grpc_async
