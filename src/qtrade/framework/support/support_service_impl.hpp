/// @file      support_service_impl.hpp
/// @brief     gRPC 异步支撑服务通用基类（AsyncService + CQ）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SUPPORT_SUPPORT_SERVICE_IMPL_HPP_
#define QTRADE_COMMON_SUPPORT_SUPPORT_SERVICE_IMPL_HPP_

#include "qtrade/framework/database/db_connection.hpp"
#include "qtrade/framework/grpc/grpc_service_host.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_framework/support/support_service.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace qtrade::common::support {

/// @brief gRPC 异步支撑服务通用基类（AsyncService + CQ；适用于 Streaming 等场景）
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

    if (grpc_host_.IsRunning()) {
      return ErrorCode::kSystemError;
    }

    if (const auto rc = grpc_host_.Start(listen_address_, connection_, service_name_); rc != ErrorCode::kSuccess) {
      state_ = SupportServiceState::kFailed;
      last_error_ = rc;
      return rc;
    }

    state_ = SupportServiceState::kReady;
    last_error_ = ErrorCode::kSuccess;
    return ErrorCode::kSuccess;
  }

  void Stop() override {
    std::lock_guard lock(mutex_);
    if (!grpc_host_.IsRunning()) {
      if (state_ == SupportServiceState::kInitializing) {
        connection_.reset();
        state_ = SupportServiceState::kTerminated;
      }
      return;
    }

    state_ = SupportServiceState::kStopping;
    grpc_host_.Shutdown();
    connection_.reset();
    state_ = SupportServiceState::kTerminated;
  }

  void Wait() override {
    grpc_host_.Wait();
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
  using GrpcHost = grpc_async::GrpcServiceHost<AsyncServiceT, HandlerT>;

  std::string service_name_;
  int default_port_;
  std::string config_path_;
  std::string listen_address_;
  ErrorCode last_error_ = ErrorCode::kSuccess;
  SupportServiceState state_ = SupportServiceState::kNew;
  mutable std::mutex mutex_;
  std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection_;
  GrpcHost grpc_host_;
};

}  // namespace qtrade::common::support

#endif  // QTRADE_COMMON_SUPPORT_SUPPORT_SERVICE_IMPL_HPP_
