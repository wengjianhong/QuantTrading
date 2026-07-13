/// @file      support_sync_service_impl.hpp
/// @brief     gRPC 同步支撑服务通用基类（Unary RPC）
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SUPPORT_SUPPORT_SYNC_SERVICE_IMPL_HPP_
#define QTRADE_COMMON_SUPPORT_SUPPORT_SYNC_SERVICE_IMPL_HPP_

#include "qtrade_framework/common/database/db_connection.hpp"
#include "qtrade_framework/common/grpc/grpc_sync_service_host.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_framework/support/support_service.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace qtrade::common::support {

/// @brief gRPC 同步支撑服务基类
/// @tparam GrpcServiceT 继承自 protobuf 生成的 xxx::Service 的实现类
template <typename GrpcServiceT>
class SupportSyncServiceImpl : public ISupportService {
 public:
  SupportSyncServiceImpl(std::string service_name, int default_port)
    : service_name_(std::move(service_name)), default_port_(default_port) {}

  ~SupportSyncServiceImpl() override {
    Stop();
  }

  SupportSyncServiceImpl(const SupportSyncServiceImpl&) = delete;
  SupportSyncServiceImpl& operator=(const SupportSyncServiceImpl&) = delete;

  ErrorCode Initialize(const std::string& config_path) override = 0;

  ErrorCode Start() override {
    std::lock_guard lock(mutex_);

    if (state_ != SupportServiceState::kInitializing || !connection_ || !connection_->IsReady()) {
      return ErrorCode::kSystemError;
    }

    if (grpc_host_.IsRunning()) {
      return ErrorCode::kSystemError;
    }

    grpc_service_ = std::make_unique<GrpcServiceT>(connection_);
    if (const auto rc = grpc_host_.Start(listen_address_, grpc_service_.get(), service_name_);
        rc != ErrorCode::kSuccess) {
      grpc_service_.reset();
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
        grpc_service_.reset();
        connection_.reset();
        state_ = SupportServiceState::kTerminated;
      }
      return;
    }

    state_ = SupportServiceState::kStopping;
    grpc_host_.Shutdown();
    grpc_service_.reset();
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
  std::string service_name_;
  int default_port_;
  std::string config_path_;
  std::string listen_address_;
  ErrorCode last_error_ = ErrorCode::kSuccess;
  SupportServiceState state_ = SupportServiceState::kNew;
  mutable std::mutex mutex_;
  std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection_;
  std::unique_ptr<GrpcServiceT> grpc_service_;
  grpc_sync::GrpcSyncServiceHost grpc_host_;
};

}  // namespace qtrade::common::support

#endif  // QTRADE_COMMON_SUPPORT_SUPPORT_SYNC_SERVICE_IMPL_HPP_
