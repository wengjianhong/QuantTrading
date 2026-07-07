/// @file      outbound_worker.cpp
/// @brief     OutboundWorker 实现
/// @author    wengjianhong
/// @date      2026-06-22
/// @copyright CC BY-NC-SA 4.0
#include "client/common/outbound_worker.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::client::detail {

ErrorCode OutboundWorker::Start(SinkFn sink, std::size_t max_queue_size) {
  if (running_.load(std::memory_order_acquire)) {
    return ErrorCode::kSystemError;
  }
  if (!sink) {
    return ErrorCode::kInternal;
  }

  max_queue_size_ = max_queue_size > 0 ? max_queue_size : 4096;
  sink_ = std::move(sink);
  running_.store(true, std::memory_order_release);
  worker_ = std::thread([this] { WorkerLoop(); });
  return ErrorCode::kSuccess;
}

void OutboundWorker::Stop() {
  if (!running_.exchange(false, std::memory_order_acq_rel)) {
    return;
  }

  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }

  std::lock_guard lock(mutex_);
  queue_.clear();
  p0_spool_.clear();
  sink_ = nullptr;
}

bool OutboundWorker::Enqueue(ReportPriority priority, std::string payload) {
  if (!running_.load(std::memory_order_acquire)) {
    return false;
  }

  {
    std::lock_guard lock(mutex_);
    if (queue_.size() >= max_queue_size_) {
      if (priority == ReportPriority::kP0Audit) {
        if (p0_spool_.size() >= kP0SpoolCapacity) {
          spdlog::error("[OutboundWorker] P0 spool full, rejecting new audit record");
          return false;
        }
        SpoolP0Locked(std::move(payload));
        cv_.notify_one();
        return true;
      }
      if (!DropOldestNonP0Locked()) {
        return false;
      }
    }
    queue_.push_back(OutboundItem{priority, std::move(payload)});
  }

  cv_.notify_one();
  return true;
}

void OutboundWorker::WorkerLoop() {
  while (running_.load(std::memory_order_acquire)) {
    OutboundItem item;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock,
               [this] { return !running_.load(std::memory_order_acquire) || !queue_.empty() || !p0_spool_.empty(); });

      if (!running_.load(std::memory_order_acquire) && queue_.empty() && p0_spool_.empty()) {
        break;
      }

      if (!p0_spool_.empty() && sink_) {
        ReplayP0SpoolLocked();
      }

      if (queue_.empty()) {
        continue;
      }

      item = std::move(queue_.front());
      queue_.pop_front();
    }

    if (sink_) {
      sink_(item.priority, item.payload);
    }
  }
}

bool OutboundWorker::DropOldestNonP0Locked() {
  for (auto it = queue_.begin(); it != queue_.end(); ++it) {
    if (it->priority != ReportPriority::kP0Audit) {
      spdlog::warn("[OutboundWorker] dropping oldest non-P0 batch (priority={})", static_cast<int>(it->priority));
      queue_.erase(it);
      return true;
    }
  }
  return false;
}

void OutboundWorker::ReplayP0SpoolLocked() {
  while (!p0_spool_.empty() && sink_) {
    const std::string& payload = p0_spool_.front();
    sink_(ReportPriority::kP0Audit, payload);
    p0_spool_.pop_front();
  }
}

void OutboundWorker::SpoolP0Locked(std::string payload) { p0_spool_.push_back(std::move(payload)); }

}  // namespace qtrade::client::detail
