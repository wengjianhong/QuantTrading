/// @file      outbound_worker.hpp
/// @brief     D 段旁路 Outbound 内存队列与专用线程
/// @details   A 段仅 Enqueue；网络/落盘在 worker 线程执行，不阻塞热路径
/// @author    wengjianhong
/// @date      2026-06-22
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_CLIENT_COMMON_OUTBOUND_WORKER_HPP_
#define QTRADE_CLIENT_COMMON_OUTBOUND_WORKER_HPP_

#include "qtrade/client/common/report_priority.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace qtrade::client::detail {

/// @brief Outbound 队列单条待上报项
struct OutboundItem {
  /// 旁路优先级
  ReportPriority priority = ReportPriority::kP3Debug;
  /// 序列化后的上报载荷
  std::string payload;
};

/// @brief 旁路上报 Outbound 工作线程（log_client / monitor_client 内部使用）
class OutboundWorker {
 public:
  /// @brief 上报 sink 回调类型
  /// @param priority 旁路优先级
  /// @param payload 上报载荷
  using SinkFn = std::function<void(ReportPriority priority, std::string_view payload)>;

  /// @brief 构造 OutboundWorker（未启动，须调用 Start）
  OutboundWorker() = default;

  /// @brief 析构并 Stop
  ~OutboundWorker() {
    Stop();
  }

  OutboundWorker(const OutboundWorker&) = delete;
  OutboundWorker& operator=(const OutboundWorker&) = delete;

  /// @brief 启动 worker 线程
  /// @param sink 在 Outbound 线程调用的上报函数
  /// @param max_queue_size 内存队列容量上限
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Start(SinkFn sink, std::size_t max_queue_size = 4096);

  /// @brief 停止 worker 并清空队列
  void Stop();

  /// @brief 异步入队（供 A 段或任意线程调用，非阻塞）
  /// @param priority 旁路优先级
  /// @param payload 上报载荷
  /// @return false 表示 P0 队列与 spool 均已满（拒写并应告警）
  bool Enqueue(ReportPriority priority, std::string payload);

  /// @brief worker 是否正在运行
  /// @return true 表示已 Start 且未 Stop
  [[nodiscard]] bool IsRunning() const {
    return running_.load(std::memory_order_acquire);
  }

  /// @brief P0 审计 spool 已满时为 true；发单准入层必须拒绝新单
  /// @return true 表示审计门禁已触发
  [[nodiscard]] bool IsAuditHalted() const {
    return audit_halted_.load(std::memory_order_acquire);
  }

 private:
  /// @brief Outbound 线程主循环
  void WorkerLoop();

  /// @brief 队列满时丢弃最旧非 P0 项（调用方已持锁）
  /// @return 是否成功腾出空间
  bool DropOldestNonP0Locked();

  /// @brief 重放 P0 spool 中积压项（调用方已持锁）
  void ReplayP0SpoolLocked();

  /// @brief 将 P0 项写入 spool（调用方已持锁）
  /// @param payload 审计载荷
  void SpoolP0Locked(std::string payload);

  /// 保护 queue_ 与 p0_spool_
  std::mutex mutex_;
  /// 通知 worker 有新项
  std::condition_variable cv_;
  /// 待上报内存队列
  std::deque<OutboundItem> queue_;
  /// P0 审计本地 spool
  std::deque<std::string> p0_spool_;
  /// Outbound 专用线程
  std::thread worker_;
  /// worker 运行标志
  std::atomic<bool> running_{false};
  /// P0 审计无法保底时的交易门禁状态
  std::atomic<bool> audit_halted_{false};
  /// 实际上报回调
  SinkFn sink_;
  /// 内存队列容量上限
  std::size_t max_queue_size_ = 4096;
  /// P0 spool 容量上限
  static constexpr std::size_t kP0SpoolCapacity = 1024;
};

}  // namespace qtrade::client::detail

#endif  // QTRADE_CLIENT_COMMON_OUTBOUND_WORKER_HPP_
