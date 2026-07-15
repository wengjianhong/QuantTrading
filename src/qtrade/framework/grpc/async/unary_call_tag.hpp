/// @file      unary_call_tag.hpp
/// @brief     Unary RPC 通用 CallTag（Server Async API）
/// @details   实现 Create → Request* → 业务处理 → Finish → respawn 状态机；Streaming 请自定义 CallTagBase 派生类
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_UNARY_CALL_TAG_HPP_
#define QTRADE_COMMON_GRPC_UNARY_CALL_TAG_HPP_

#include "qtrade/framework/grpc/async/call_tag_base.hpp"

#include <grpcpp/grpcpp.h>

#include <functional>

namespace qtrade::common::grpc_async {

/// @brief Unary RPC 通用 CallTag
/// @tparam AsyncServiceT protobuf 生成的 gRPC AsyncService 类型
/// @tparam HandlerT RPC 处理器类型
/// @tparam RequestT 请求 message 类型
/// @tparam ResponseT 响应 message 类型
template <typename AsyncServiceT, typename HandlerT, typename RequestT, typename ResponseT>
class UnaryCallTag final : public CallTagBase {
 public:
  /// @brief AsyncService::RequestXxx 成员函数指针类型
  using RequestMethod = void (AsyncServiceT::*)(grpc::ServerContext*,
                                                RequestT*,
                                                grpc::ServerAsyncResponseWriter<ResponseT>*,
                                                grpc::CompletionQueue*,
                                                grpc::ServerCompletionQueue*,
                                                void*);
  /// @brief 业务处理回调：填充 response 并返回 Status
  using HandlerFn = std::function<grpc::Status(HandlerT*, const RequestT&, ResponseT*)>;
  /// @brief 本请求结束后用于再挂起一个同类 CallTag 的回调
  using RespawnFn = std::function<void(HandlerT*)>;

  /// @brief 构造 Unary CallTag 并立即进入 Create/Request 流程
  /// @param handler 业务处理器（不可为 nullptr；生命周期须覆盖本 tag）
  /// @param service AsyncService 实例（不可为 nullptr）
  /// @param cq 服务端 CompletionQueue（不可为 nullptr）
  /// @param request_method AsyncService::RequestXxx 成员函数
  /// @param handler_fn 收到请求后的业务回调
  /// @param respawn_fn Finish 后再次 Spawn 同类请求的回调
  UnaryCallTag(HandlerT* handler,
               AsyncServiceT* service,
               grpc::ServerCompletionQueue* cq,
               RequestMethod request_method,
               HandlerFn handler_fn,
               RespawnFn respawn_fn)
    : handler_(handler),
      service_(service),
      cq_(cq),
      request_method_(request_method),
      handler_fn_(std::move(handler_fn)),
      respawn_fn_(std::move(respawn_fn)),
      responder_(&ctx_) {
    Proceed(true);
  }

  /// @brief 推进 Unary 状态机
  /// @param ok CQ 事件是否成功；Create 之后失败则自删
  void Proceed(bool ok) override {
    if (!ok) {
      delete this;
      return;
    }

    // 1. CREATE：挂起 RequestXxx，等待下一事件进入 PROCESS
    if (status_ == CallStatus::kCreate) {
      status_ = CallStatus::kProcess;
      (service_->*request_method_)(&ctx_, &request_, &responder_, cq_, cq_, this);
      return;
    }

    // 2. PROCESS：执行业务并 Finish
    if (status_ == CallStatus::kProcess) {
      status_ = CallStatus::kFinish;
      const grpc::Status status = handler_fn_(handler_, request_, &response_);
      responder_.Finish(response_, status, this);
      return;
    }

    // 3. FINISH：respawn 下一个 CallTag 并自删
    respawn_fn_(handler_);
    delete this;
  }

 private:
  /// @brief Unary CallTag 内部状态
  enum class CallStatus {
    /// 初始：即将调用 RequestXxx
    kCreate = 0,
    /// 已收到请求：执行业务并 Finish
    kProcess = 1,
    /// Finish 完成：respawn 后销毁
    kFinish = 2,
  };

  /// 业务处理器
  HandlerT* handler_;
  /// AsyncService
  AsyncServiceT* service_;
  /// 服务端 CQ
  grpc::ServerCompletionQueue* cq_;
  /// RequestXxx 函数指针
  RequestMethod request_method_;
  /// 业务回调
  HandlerFn handler_fn_;
  /// 结束后再挂起请求的回调
  RespawnFn respawn_fn_;
  /// 本请求上下文
  grpc::ServerContext ctx_;
  /// 请求消息
  RequestT request_;
  /// 响应消息
  ResponseT response_;
  /// Unary 响应写端
  grpc::ServerAsyncResponseWriter<ResponseT> responder_;
  /// 当前状态机状态
  CallStatus status_ = CallStatus::kCreate;
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_UNARY_CALL_TAG_HPP_
