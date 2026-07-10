/// @file      monitor_client.cpp
/// @brief     监控指标客户端实现
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "client/monitor_client/monitor_client.hpp"

#include "client/common/outbound_worker.hpp"

#include "client/common/report_priority.hpp"

#include <sstream>

namespace qtrade::client {

MonitorClient::MonitorClient() = default;

MonitorClient::~MonitorClient() {
  Shutdown();
}

ErrorCode MonitorClient::Init(std::string_view endpoint) {
  if (initialized_) {
    return ErrorCode::kSystemError;
  }

  endpoint_ = std::string(endpoint);
  worker_ = std::make_unique<detail::OutboundWorker>();

  // MVP stub：远程上报 no-op；Outbound 队列仍可用于后续 gRPC 接入
  const ErrorCode rc = worker_->Start([](ReportPriority /*priority*/, std::string_view /*payload*/) {});

  if (rc != ErrorCode::kSuccess) {
    worker_.reset();
    return rc;
  }

  initialized_ = true;
  return ErrorCode::kSuccess;
}

void MonitorClient::Shutdown() {
  if (!initialized_) {
    return;
  }
  if (worker_) {
    worker_->Stop();
    worker_.reset();
  }
  initialized_ = false;
}

void MonitorClient::Counter(std::string_view name, double value) {
  if (!initialized_ || !worker_) {
    return;
  }
  std::ostringstream oss;
  oss << "counter|" << name << '|' << value;
  worker_->Enqueue(ReportPriority::kP2Metrics, oss.str());
}

void MonitorClient::Gauge(std::string_view name, double value) {
  if (!initialized_ || !worker_) {
    return;
  }
  std::ostringstream oss;
  oss << "gauge|" << name << '|' << value;
  worker_->Enqueue(ReportPriority::kP2Metrics, oss.str());
}

}  // namespace qtrade::client
