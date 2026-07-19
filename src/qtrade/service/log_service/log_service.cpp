/// @file      log_service.cpp
/// @brief     日志支撑服务 MVP 实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/log_service/log_service.hpp"

#include "qtrade/common/config/qtrade_log_service_config.hpp"
#include "qtrade/common/json/json_util.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::service {

LogService::LogService() {
  status_.service_name = "qtrade_log_service";
  status_.state = qtrade::common::support::SupportServiceState::kNew;
}

LogService::~LogService() {
  Stop();
}

ErrorCode LogService::Initialize(const std::string& config_path) {
  std::lock_guard lock(mutex_);
  if (status_.state != qtrade::common::support::SupportServiceState::kNew &&
      status_.state != qtrade::common::support::SupportServiceState::kTerminated) {
    return ErrorCode::kSystemError;
  }

  status_.state = qtrade::common::support::SupportServiceState::kInitializing;
  status_.config_path = config_path;

  const auto config_node = qtrade::common::LoadJsonFile(config_path);
  if (!config_node.has_value()) {
    status_.state = qtrade::common::support::SupportServiceState::kFailed;
    status_.last_error = ErrorCode::kNotFound;
    status_.last_error_message = "config file not found";
    return status_.last_error;
  }

  const auto loaded = qtrade::common::config::ParseQtradeLogServiceConfig(*config_node);
  if (!loaded.has_value()) {
    status_.state = qtrade::common::support::SupportServiceState::kFailed;
    status_.last_error = ErrorCode::kInternal;
    status_.last_error_message = "invalid log service config";
    return status_.last_error;
  }

  status_.listen_address = loaded->grpc.Address();
  status_.last_error = ErrorCode::kSuccess;
  status_.last_error_message.clear();
  spdlog::info("[LogService] initialized, listen={}", status_.listen_address);
  return ErrorCode::kSuccess;
}

ErrorCode LogService::Start() {
  std::lock_guard lock(mutex_);
  if (status_.state != qtrade::common::support::SupportServiceState::kInitializing) {
    return ErrorCode::kSystemError;
  }
  // MVP：暂未挂载 gRPC ingest；进程生命周期已对齐 ISupportService
  stop_requested_ = false;
  status_.state = qtrade::common::support::SupportServiceState::kReady;
  spdlog::info("[LogService] started (MVP stub, gRPC ingest TODO)");
  return ErrorCode::kSuccess;
}

void LogService::Stop() {
  std::lock_guard lock(mutex_);
  if (status_.state == qtrade::common::support::SupportServiceState::kTerminated ||
      status_.state == qtrade::common::support::SupportServiceState::kNew) {
    return;
  }
  status_.state = qtrade::common::support::SupportServiceState::kStopping;
  stop_requested_ = true;
  status_.state = qtrade::common::support::SupportServiceState::kTerminated;
  stop_cv_.notify_all();
  spdlog::info("[LogService] stopped");
}

void LogService::Wait() {
  std::unique_lock lock(mutex_);
  stop_cv_.wait(lock, [this] { return stop_requested_; });
}

qtrade::common::support::SupportServiceStatus LogService::GetStatus() const {
  std::lock_guard lock(mutex_);
  return status_;
}

}  // namespace qtrade::service
