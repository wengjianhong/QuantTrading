/// @file      support_service_impl.hpp
/// @brief     gRPC 异步支撑服务通用基类（AsyncService + CQ）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SUPPORT_SUPPORT_SERVICE_IMPL_HPP_
#define QTRADE_COMMON_SUPPORT_SUPPORT_SERVICE_IMPL_HPP_

#include "qtrade/framework/database/db_connection.hpp"
#include "qtrade/framework/grpc/grpc_async_server.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_framework/support/support_service.hpp>

#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace qtrade::common::support {

/// @brief gRPC 异步支撑服务通用基类（AsyncService + CQ；适用于 Streaming 等场景）
/// @tparam AsyncServiceT protobuf 生成的 gRPC AsyncService 类型
/// @tparam HandlerT RPC 异步处理器，需提供 Init / Start / Shutdown
template <typename AsyncServiceT, typename HandlerT>
class SupportAsyncServiceImpl : public ISupportService {
 public:
  SupportAsyncServiceImpl(std::string service_name, int default_port)
    : service_name_(std::move(service_name)), default_port_(default_port) {}

  ~SupportAsyncServiceImpl() override {
    Stop();
  }

  SupportAsyncServiceImpl(const SupportAsyncServiceImpl&) = delete;
  SupportAsyncServiceImpl& operator=(const SupportAsyncServiceImpl&) = delete;

  ErrorCode Initialize(const std::string& config_path) override = 0;

  ErrorCode Start() override {
    std::lock_guard lock(mutex_);

    if (state_ != SupportServiceState::kInitializing || !connection_ || !connection_->IsReady()) {
      return ErrorCode::kSystemError;
    }

    if (grpc_server_ && grpc_server_->IsRunning()) {
      return ErrorCode::kSystemError;
    }

    grpc_server_ = std::make_unique<grpc_async::GrpcAsyncServer>();
    handler_ = std::make_unique<HandlerT>();

    grpc_async::GrpcAsyncServer::Options opts;
    opts.listen_address = listen_address_;
    opts.cq_thread_count = 1;

    if (const auto rc = grpc_server_->Start(opts, &async_service_); rc != ErrorCode::kSuccess) {
      handler_.reset();
      grpc_server_.reset();
      state_ = SupportServiceState::kFailed;
      last_error_ = rc;
      return rc;
    }

    handler_->Init(&async_service_, grpc_server_->CompletionQueue(), connection_);
    handler_->Start();

    state_ = SupportServiceState::kReady;
    last_error_ = ErrorCode::kSuccess;
    spdlog::info("[{}] listening on {} (async+cq)", service_name_, listen_address_);
    return ErrorCode::kSuccess;
  }

  void Stop() override {
    std::lock_guard lock(mutex_);
    if (!grpc_server_ || !grpc_server_->IsRunning()) {
      if (state_ == SupportServiceState::kInitializing) {
        connection_.reset();
        state_ = SupportServiceState::kTerminated;
      }
      return;
    }

    state_ = SupportServiceState::kStopping;
    if (handler_) {
      handler_->Shutdown();
      handler_.reset();
    }
    grpc_server_->Shutdown();
    connection_.reset();
    state_ = SupportServiceState::kTerminated;
  }

  void Wait() override {
    if (grpc_server_) {
      grpc_server_->Wait();
      grpc_server_.reset();
    }
  }

  [[nodiscard]] SupportServiceStatus GetStatus() const override {
    std::lock_guard lock(mutex_);
    return SupportServiceStatus{
      .service_name = service_name_,
      .config_path = config_path_,
      .listen_address = listen_address_,
      .last_error_message = {},
      .last_error = last_error_,
      .state = state_,
    };
  }

 protected:
  std::string service_name_;
  int default_port_;
  std::string config_path_;
  std::string listen_address_;
  ErrorCode last_error_ = ErrorCode::kSuccess;
  SupportServiceState state_ = SupportServiceState::kNew;
  mutable std::mutex mutex_;
  std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection_;
  AsyncServiceT async_service_;
  std::unique_ptr<HandlerT> handler_;
  std::unique_ptr<grpc_async::GrpcAsyncServer> grpc_server_;
};

}  // namespace qtrade::common::support

#endif  // QTRADE_COMMON_SUPPORT_SUPPORT_SERVICE_IMPL_HPP_
