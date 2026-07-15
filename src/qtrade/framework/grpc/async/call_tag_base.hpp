/// @file      call_tag_base.hpp
/// @brief     gRPC Async CompletionQueue CallTag 基类
/// @details   CQ 事件以 void* tag 投递；轮询线程将其转为 CallTagBase 并调用 Proceed
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_CALL_TAG_BASE_HPP_
#define QTRADE_COMMON_GRPC_CALL_TAG_BASE_HPP_

namespace qtrade::common::grpc_async {

/// @brief CompletionQueue tag 对应的异步 RPC 状态机入口
class CallTagBase {
 public:
  /// @brief 虚析构，保证派生 CallTag 正确释放
  virtual ~CallTagBase() = default;

  /// @brief CQ 事件到达时由 CompletionQueueLoop 调用
  /// @param ok false 表示该事件对应的操作失败或 RPC 已取消
  virtual void Proceed(bool ok) = 0;
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_CALL_TAG_BASE_HPP_
