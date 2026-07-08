/// @file      unary_call_tag.hpp
/// @brief     Unary RPC 通用 CallTag（Server Async API）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_GRPC_UNARY_CALL_TAG_HPP_
#define QTRADE_COMMON_GRPC_UNARY_CALL_TAG_HPP_

#include "common/grpc/call_tag_base.hpp"

#include <functional>

#include <grpcpp/grpcpp.h>

namespace qtrade::common::grpc_async {

/// @brief Unary RPC 通用 CallTag
/// @tparam AsyncServiceT protobuf 生成的 gRPC AsyncService 类型
/// @tparam HandlerT RPC 处理器类型
/// @tparam RequestT 请求 message 类型
/// @tparam ResponseT 响应 message 类型
template <typename AsyncServiceT, typename HandlerT, typename RequestT, typename ResponseT>
class UnaryCallTag final : public CallTagBase {
 public:
  using RequestMethod = void (AsyncServiceT::*)(grpc::ServerContext*,
                                                RequestT*,
                                                grpc::ServerAsyncResponseWriter<ResponseT>*,
                                                grpc::CompletionQueue*,
                                                grpc::ServerCompletionQueue*,
                                                void*);
  using HandlerFn = std::function<grpc::Status(HandlerT*, const RequestT&, ResponseT*)>;
  using RespawnFn = std::function<void(HandlerT*)>;

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

  void Proceed(bool ok) override {
    if (!ok) {
      delete this;
      return;
    }

    if (status_ == CallStatus::kCreate) {
      status_ = CallStatus::kProcess;
      (service_->*request_method_)(&ctx_, &request_, &responder_, cq_, cq_, this);
      return;
    }

    if (status_ == CallStatus::kProcess) {
      status_ = CallStatus::kFinish;
      const grpc::Status status = handler_fn_(handler_, request_, &response_);
      responder_.Finish(response_, status, this);
      return;
    }

    respawn_fn_(handler_);
    delete this;
  }

 private:
  enum class CallStatus { kCreate, kProcess, kFinish };

  HandlerT* handler_;
  AsyncServiceT* service_;
  grpc::ServerCompletionQueue* cq_;
  RequestMethod request_method_;
  HandlerFn handler_fn_;
  RespawnFn respawn_fn_;
  grpc::ServerContext ctx_;
  RequestT request_;
  ResponseT response_;
  grpc::ServerAsyncResponseWriter<ResponseT> responder_;
  CallStatus status_ = CallStatus::kCreate;
};

}  // namespace qtrade::common::grpc_async

#endif  // QTRADE_COMMON_GRPC_UNARY_CALL_TAG_HPP_
