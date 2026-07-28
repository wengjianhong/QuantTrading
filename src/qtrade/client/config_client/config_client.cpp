/// @file      config_client.cpp
/// @brief     ConfigService gRPC 客户端实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/client/config_client/config_client.hpp"

#include <qtrade/proto/config/v1/config.grpc.pb.h>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <utility>

namespace qtrade::client {
namespace {

[[nodiscard]] std::chrono::system_clock::time_point DeadlineFrom(const qtrade::common::config::ServiceConfig& cfg) {
  const int timeout_ms = cfg.timeout_ms > 0 ? cfg.timeout_ms : 5000;
  return std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms);
}

}  // namespace

struct ConfigClient::Impl {
  ConfigClientOptions options;
  std::shared_ptr<grpc::Channel> channel;
  std::unique_ptr<qtrade::config::v1::ConfigService::Stub> stub;
  std::thread watch_thread;
  std::atomic<bool> watch_running{false};
  bool initialized = false;
};

ConfigClient::ConfigClient() : impl_(std::make_unique<Impl>()) {}

ConfigClient::~ConfigClient() {
  Shutdown();
}

ErrorCode ConfigClient::Init(const ConfigClientOptions& options) {
  if (impl_->initialized) {
    return ErrorCode::kSystemError;
  }
  if (options.service_config.host.empty() || options.service_config.port <= 0) {
    return ErrorCode::kInternalError;
  }
  impl_->options = options;
  impl_->channel = grpc::CreateChannel(options.service_config.Address(), grpc::InsecureChannelCredentials());
  impl_->stub = qtrade::config::v1::ConfigService::NewStub(impl_->channel);
  impl_->initialized = true;
  return ErrorCode::kSuccess;
}

bool ConfigClient::IsInitialized() const {
  return impl_->initialized;
}

void ConfigClient::Shutdown() {
  impl_->watch_running.store(false, std::memory_order_release);
  if (impl_->watch_thread.joinable()) {
    impl_->watch_thread.join();
  }
  impl_->stub.reset();
  impl_->channel.reset();
  impl_->initialized = false;
}

ErrorCode ConfigClient::GetEngineConfig(const qtrade::config::v1::GetEngineConfigRequest& request,
                                        qtrade::config::v1::GetEngineConfigResponse& response) {
  if (!impl_->initialized || !impl_->stub) {
    return ErrorCode::kNotInitialized;
  }
  grpc::ClientContext context;
  context.set_deadline(DeadlineFrom(impl_->options.service_config));
  const grpc::Status status = impl_->stub->GetEngineConfig(&context, request, &response);
  if (!status.ok()) {
    spdlog::warn("[ConfigClient] GetEngineConfig failed: {}", status.error_message());
    return ErrorCode::kTimeout;
  }
  return ErrorCode::kSuccess;
}

ErrorCode ConfigClient::SubscribeEngineConfig(const qtrade::config::v1::SubscribeEngineConfigRequest& request,
                                              SubscribeHandler on_message) {
  if (!impl_->initialized || !impl_->stub) {
    return ErrorCode::kNotInitialized;
  }
  if (!on_message) {
    return ErrorCode::kInternalError;
  }
  if (impl_->watch_running.load(std::memory_order_acquire)) {
    return ErrorCode::kSystemError;
  }

  impl_->watch_running.store(true, std::memory_order_release);
  impl_->watch_thread = std::thread([this, request, handler = std::move(on_message)]() mutable {
    int backoff_ms = 500;
    constexpr int kMaxBackoffMs = 30'000;
    auto active_request = request;

    while (impl_->watch_running.load(std::memory_order_acquire)) {
      if (!impl_->stub) {
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        continue;
      }

      grpc::ClientContext context;
      qtrade::config::v1::SubscribeEngineConfigResponse response;
      std::unique_ptr<grpc::ClientReader<qtrade::config::v1::SubscribeEngineConfigResponse>> reader(
        impl_->stub->SubscribeEngineConfig(&context, active_request));

      while (impl_->watch_running.load(std::memory_order_acquire) && reader->Read(&response)) {
        backoff_ms = 500;
        if (response.engine().version() > active_request.since_version()) {
          active_request.set_since_version(response.engine().version());
        }
        handler(response);
      }

      const grpc::Status status = reader->Finish();
      if (!impl_->watch_running.load(std::memory_order_acquire)) {
        break;
      }
      if (!status.ok() && status.error_code() != grpc::StatusCode::CANCELLED) {
        spdlog::warn("[ConfigClient] SubscribeEngineConfig disconnected: {}", status.error_message());
      }
      if (!status.ok() && impl_->watch_running.load(std::memory_order_acquire)) {
        spdlog::info("[ConfigClient] reconnecting in {} ms...", backoff_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        backoff_ms = std::min(backoff_ms * 2, kMaxBackoffMs);
      }
    }
  });

  return ErrorCode::kSuccess;
}

}  // namespace qtrade::client
