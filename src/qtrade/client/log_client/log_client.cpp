/// @file      log_client.cpp
/// @brief     日志客户端实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/client/log_client/log_client.hpp"

#include "qtrade/client/common/outbound_worker.hpp"

#include <spdlog/spdlog.h>

#include <sstream>

namespace qtrade::client {

LogClient::LogClient() = default;

LogClient::~LogClient() {
  Shutdown();
}

ErrorCode LogClient::Init(std::string_view topic) {
  if (initialized_) {
    return ErrorCode::kSystemError;
  }

  topic_ = std::string(topic);
  worker_ = std::make_unique<detail::OutboundWorker>();

  const std::string topic_copy = topic_;
  const ErrorCode rc = worker_->Start([topic_copy](ReportPriority priority, std::string_view payload) {
    // MVP stub：本地 spdlog；远程 gRPC 上报可替换此 sink
    if (priority == ReportPriority::kP0Audit) {
      spdlog::info("[audit][{}] {}", topic_copy, payload);
      return;
    }
    if (priority == ReportPriority::kP2Metrics || priority == ReportPriority::kP3Debug) {
      spdlog::debug("[{}] {}", topic_copy, payload);
      return;
    }
    spdlog::info("[{}] {}", topic_copy, payload);
  });

  if (rc != ErrorCode::kSuccess) {
    worker_.reset();
    return rc;
  }

  initialized_ = true;
  return ErrorCode::kSuccess;
}

void LogClient::Shutdown() {
  if (!initialized_) {
    return;
  }
  if (worker_) {
    worker_->Stop();
    worker_.reset();
  }
  initialized_ = false;
}

void LogClient::Emit(std::string_view level, std::string_view message) {
  if (!initialized_ || !worker_) {
    return;
  }

  std::ostringstream oss;
  oss << level << '|' << message;
  worker_->Enqueue(LevelToPriority(level), oss.str());
}

void LogClient::EmitAudit(std::string_view message) {
  if (!initialized_ || !worker_) {
    return;
  }
  worker_->Enqueue(ReportPriority::kP0Audit, std::string(message));
}

ReportPriority LogClient::LevelToPriority(std::string_view level) {
  if (level == "audit") {
    return ReportPriority::kP0Audit;
  }
  if (level == "error" || level == "warn") {
    return ReportPriority::kP1Business;
  }
  if (level == "debug") {
    return ReportPriority::kP3Debug;
  }
  return ReportPriority::kP2Metrics;
}

}  // namespace qtrade::client
