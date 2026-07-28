/// @file      account_risk_release_worker.cpp
/// @brief     账户预占释放可靠工作器实现
/// @details   outbox 落盘、回放未确认任务，并由独立线程重试 Release
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/risk/account_risk_release_worker.hpp"

#include <qtrade/proto/account_risk/v1/account_risk.pb.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <unordered_map>

namespace qtrade::engine::risk {
namespace {

/// @brief 回放 outbox 文件
/// @param path 文件路径
/// @param next_sequence 输出下一序号
/// @return 尚未收到 ack 的任务
std::unordered_map<std::string, int> ReplayOutbox(const std::string& path, std::uint64_t& next_sequence) {
  std::unordered_map<std::string, int> pending;
  std::ifstream input(path);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    try {
      const auto value = nlohmann::json::parse(line);
      const auto sequence = value.value("sequence", std::uint64_t{0});
      next_sequence = std::max(next_sequence, sequence + 1);
      const std::string order_id = value.value("order_id", "");
      if (order_id.empty()) {
        continue;
      }
      if (value.value("kind", "") == "pending") {
        pending[order_id] = value.value("reason", 0);
      } else if (value.value("kind", "") == "ack") {
        pending.erase(order_id);
      }
    } catch (const std::exception& error) {
      spdlog::error("[AccountRiskReleaseWorker] ignored invalid outbox record: {}", error.what());
    }
  }
  return pending;
}

}  // namespace

AccountRiskReleaseWorker::~AccountRiskReleaseWorker() {
  Stop();
}

ErrorCode AccountRiskReleaseWorker::Initialize(client::AccountRiskClient* client,
                                               const std::string& path,
                                               bool sync_on_append,
                                               std::string tenant_id,
                                               std::string account_id) {
  if (client == nullptr || !client->IsInitialized() || path.empty() || tenant_id.empty() || account_id.empty()) {
    return ErrorCode::kNotInitialized;
  }

  std::lock_guard lock(mutex_);
  if (fd_ >= 0 || running_) {
    return ErrorCode::kSystemError;
  }
  // 1. 确保 outbox 目录存在并回放未确认任务
  const std::filesystem::path outbox_path(path);
  if (outbox_path.has_parent_path()) {
    std::error_code error;
    std::filesystem::create_directories(outbox_path.parent_path(), error);
    if (error) {
      return ErrorCode::kSystemError;
    }
  }

  path_ = path;
  sync_on_append_ = sync_on_append;
  tenant_id_ = std::move(tenant_id);
  account_id_ = std::move(account_id);
  next_sequence_ = 1;
  const auto recovered = ReplayOutbox(path_, next_sequence_);
  fd_ = ::open(path_.c_str(), O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, 0640);
  if (fd_ < 0) {
    path_.clear();
    return ErrorCode::kSystemError;
  }

  // 2. 将恢复出的 pending 任务重新入队
  client_ = client;
  for (const auto& [order_id, reason] : recovered) {
    tasks_.push_back(Task{order_id, reason});
    pending_order_ids_.insert(order_id);
  }
  return ErrorCode::kSuccess;
}

void AccountRiskReleaseWorker::Start() {
  std::lock_guard lock(mutex_);
  if (running_ || fd_ < 0 || client_ == nullptr) {
    return;
  }
  running_ = true;
  worker_ = std::thread([this] { Run(); });
  cv_.notify_all();
}

void AccountRiskReleaseWorker::Stop() {
  {
    std::lock_guard lock(mutex_);
    running_ = false;
  }
  // 1. 唤醒工作线程并等待退出
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
  // 2. 关闭 outbox 文件句柄并清空内存状态
  std::lock_guard lock(mutex_);
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  tasks_.clear();
  pending_order_ids_.clear();
  client_ = nullptr;
}

ErrorCode AccountRiskReleaseWorker::Enqueue(const std::string& order_id, int reason) {
  if (order_id.empty()) {
    return ErrorCode::kSystemError;
  }
  std::lock_guard lock(mutex_);
  if (fd_ < 0 || !running_) {
    return ErrorCode::kNotInitialized;
  }
  // 已在 pending 集合中则视为幂等成功
  if (pending_order_ids_.contains(order_id)) {
    return ErrorCode::kSuccess;
  }

  // 先落盘 pending 事实，再入内存队列
  Task task{order_id, reason};
  if (const auto result = AppendRecord("pending", task); result != ErrorCode::kSuccess) {
    return result;
  }
  pending_order_ids_.insert(order_id);
  tasks_.push_back(std::move(task));
  cv_.notify_one();
  return ErrorCode::kSuccess;
}

std::size_t AccountRiskReleaseWorker::PendingCount() const {
  std::lock_guard lock(mutex_);
  return pending_order_ids_.size();
}

ErrorCode AccountRiskReleaseWorker::AppendRecord(const char* kind, const Task& task) {
  if (fd_ < 0) {
    return ErrorCode::kNotInitialized;
  }
  nlohmann::json value = {
    {"sequence", next_sequence_},
    {"kind", kind},
    {"order_id", task.order_id},
    {"reason", task.reason},
  };
  std::string line = value.dump();
  line.push_back('\n');

  // 完整写出一行，必要时 fsync
  std::size_t offset = 0;
  while (offset < line.size()) {
    const ssize_t written = ::write(fd_, line.data() + offset, line.size() - offset);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return ErrorCode::kSystemError;
    }
    offset += static_cast<std::size_t>(written);
  }
  if (sync_on_append_ && ::fsync(fd_) != 0) {
    return ErrorCode::kSystemError;
  }
  ++next_sequence_;
  return ErrorCode::kSuccess;
}

void AccountRiskReleaseWorker::Run() {
  while (true) {
    // 1. 等待并取出队头任务
    Task task;
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return !running_ || !tasks_.empty(); });
      if (!running_) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop_front();
    }

    // 2. 调用远程 Release；成功则写 ack，失败则退避后重入队
    qtrade::account_risk::v1::ReleaseOrderRequest release_request;
    release_request.set_tenant_id(tenant_id_);
    release_request.set_account_id(account_id_);
    release_request.set_order_id(task.order_id);
    release_request.set_reason(static_cast<qtrade::account_risk::v1::ReleaseOrderRequest::Reason>(task.reason));
    qtrade::account_risk::v1::ReleaseOrderResponse response;
    const auto result = client_->ReleaseOrder(release_request, response);
    if (result == ErrorCode::kSuccess && response.released()) {
      std::lock_guard lock(mutex_);
      if (AppendRecord("ack", task) == ErrorCode::kSuccess) {
        pending_order_ids_.erase(task.order_id);
        continue;
      }
    }

    std::unique_lock lock(mutex_);
    if (!running_) {
      return;
    }
    cv_.wait_for(lock, std::chrono::milliseconds(50), [this] { return !running_; });
    if (running_) {
      tasks_.push_back(std::move(task));
    }
  }
}

}  // namespace qtrade::engine::risk
