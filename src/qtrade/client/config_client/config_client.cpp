/// @file      config_client.cpp
/// @brief     配置管理客户端实现（gRPC 出站）
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

struct ConfigClient::Impl {
  /// 连接参数
  ConfigClientOptions options;
  /// 全量快照回调
  SnapshotHandler on_snapshot;
  /// gRPC 通道
  std::shared_ptr<grpc::Channel> channel;
  /// gRPC 存根
  std::unique_ptr<qtrade::config::v1::ConfigService::Stub> stub;
  /// SubscribeEngineConfig 控制线程
  std::thread watch_thread;
  /// Watch 线程运行标志
  std::atomic<bool> watch_running{false};
  /// 已应用配置版本
  std::atomic<std::uint64_t> version{0};
  /// 是否已完成 Init
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
  if (options.server_address.empty()) {
    return ErrorCode::kInternalError;
  }

  impl_->options = options;
  impl_->channel = grpc::CreateChannel(options.server_address, grpc::InsecureChannelCredentials());
  impl_->stub = qtrade::config::v1::ConfigService::NewStub(impl_->channel);
  impl_->initialized = true;
  return ErrorCode::kSuccess;
}

void ConfigClient::ApplyConfig(const qtrade::config::v1::EngineConfig& config) {
  impl_->version.store(config.version(), std::memory_order_release);
  if (impl_->on_snapshot) {
    impl_->on_snapshot(config);
  }
}

ErrorCode ConfigClient::FetchSnapshot() {
  if (!impl_->initialized || !impl_->stub) {
    return ErrorCode::kNotInitialized;
  }

  qtrade::config::v1::GetEngineConfigRequest request;
  request.set_engine_id(impl_->options.engine_id);

  qtrade::config::v1::GetEngineConfigResponse response;
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

  const grpc::Status status = impl_->stub->GetEngineConfig(&context, request, &response);
  if (!status.ok()) {
    spdlog::warn("[ConfigClient] GetEngineConfig failed: {}", status.error_message());
    return ErrorCode::kTimeout;
  }

  ApplyConfig(response.engine());
  spdlog::info("[ConfigClient] config loaded, version={}, strategies={}",
               response.engine().version(),
               response.engine().strategies_size());
  return ErrorCode::kSuccess;
}

ErrorCode ConfigClient::StartWatch() {
  if (!impl_->initialized) {
    return ErrorCode::kNotInitialized;
  }
  if (impl_->watch_running.load(std::memory_order_acquire)) {
    return ErrorCode::kSystemError;
  }

  impl_->watch_running.store(true, std::memory_order_release);
  impl_->watch_thread = std::thread([this] {
    int backoff_ms = 500;
    constexpr int kMaxBackoffMs = 30'000;

    while (impl_->watch_running.load(std::memory_order_acquire)) {
      if (!impl_->stub) {
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        continue;
      }

      qtrade::config::v1::SubscribeEngineConfigRequest request;
      request.set_engine_id(impl_->options.engine_id);
      request.set_since_version(impl_->version.load(std::memory_order_acquire));

      grpc::ClientContext context;
      qtrade::config::v1::SubscribeEngineConfigResponse response;
      std::unique_ptr<grpc::ClientReader<qtrade::config::v1::SubscribeEngineConfigResponse>> reader(
        impl_->stub->SubscribeEngineConfig(&context, request));

      while (impl_->watch_running.load(std::memory_order_acquire) && reader->Read(&response)) {
        backoff_ms = 500;
        ApplyConfig(response.engine());
        spdlog::debug("[ConfigClient] applied config version={}, strategies={}",
                      response.engine().version(),
                      response.engine().strategies_size());
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

void ConfigClient::Shutdown() {
  impl_->watch_running.store(false, std::memory_order_release);
  if (impl_->watch_thread.joinable()) {
    impl_->watch_thread.join();
  }
  impl_->stub.reset();
  impl_->channel.reset();
  impl_->initialized = false;
}

void ConfigClient::SetOnSnapshot(SnapshotHandler handler) {
  impl_->on_snapshot = std::move(handler);
}

std::uint64_t ConfigClient::Version() const {
  return impl_->version.load(std::memory_order_acquire);
}

bool ConfigClient::IsInitialized() const {
  return impl_->initialized;
}

}  // namespace qtrade::client
