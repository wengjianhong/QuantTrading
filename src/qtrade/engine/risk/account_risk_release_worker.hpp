/// @file      account_risk_release_worker.hpp
/// @brief     账户预占释放可靠工作器
/// @details   将 E 段 Release 写入本地 outbox，由独立线程重试，避免阻塞 Lane-T
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_RISK_ACCOUNT_RISK_RELEASE_WORKER_HPP_
#define QTRADE_ENGINE_RISK_ACCOUNT_RISK_RELEASE_WORKER_HPP_

#include "qtrade/client/account_risk_client/account_risk_client.hpp"

#include <qtrade/error_code/error_codes.hpp>

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

namespace qtrade::engine::risk {

/// @brief E 段 Release 持久化重试工作器
class AccountRiskReleaseWorker {
 public:
  /// @brief 析构并停止工作线程
  ~AccountRiskReleaseWorker();

  /// @brief 禁止拷贝构造
  AccountRiskReleaseWorker(const AccountRiskReleaseWorker&) = delete;

  /// @brief 禁止拷贝赋值
  AccountRiskReleaseWorker& operator=(const AccountRiskReleaseWorker&) = delete;

  /// @brief 构造未初始化的工作器
  AccountRiskReleaseWorker() = default;

  /// @brief 初始化 outbox 并回放未确认任务
  /// @param client 账户风控客户端
  /// @param path outbox JSON Lines 文件路径
  /// @param sync_on_append 每条事实是否 fsync
  /// @return 成功返回 kSuccess
  ErrorCode Initialize(client::AccountRiskClient* client, const std::string& path, bool sync_on_append);

  /// @brief 启动重试线程
  void Start();

  /// @brief 停止线程并保留未确认任务供下次回放
  void Stop();

  /// @brief 可靠提交释放请求
  /// @param order_id 全局订单 ID
  /// @param reason ReleaseOrderRequest::Reason 整数值
  /// @return outbox 落盘成功返回 kSuccess
  ErrorCode Enqueue(const std::string& order_id, int reason);

  /// @brief 查询待确认任务数量
  /// @return 未确认 order_id 数量
  [[nodiscard]] std::size_t PendingCount() const;

 private:
  /// @brief 单个 outbox 任务
  struct Task {
    /// 全局订单 ID
    std::string order_id;
    /// 释放原因
    int reason = 0;
  };

  /// @brief 写入一条 outbox 记录
  /// @param kind pending 或 ack
  /// @param task 释放任务
  /// @return 成功返回 kSuccess
  ErrorCode AppendRecord(const char* kind, const Task& task);

  /// @brief 工作线程循环
  void Run();

  /// 账户硬风控客户端
  client::AccountRiskClient* client_ = nullptr;
  /// outbox 文件路径
  std::string path_;
  /// outbox 文件描述符
  int fd_ = -1;
  /// 每条追加是否强制落盘
  bool sync_on_append_ = true;
  /// 下一个记录序号
  std::uint64_t next_sequence_ = 1;
  /// 待处理任务
  std::deque<Task> tasks_;
  /// 已持久化但未确认的订单
  std::unordered_set<std::string> pending_order_ids_;
  /// 保护文件、队列和状态
  mutable std::mutex mutex_;
  /// 工作线程条件变量
  std::condition_variable cv_;
  /// 工作线程
  std::thread worker_;
  /// 是否运行
  bool running_ = false;
};

}  // namespace qtrade::engine::risk

#endif  // QTRADE_ENGINE_RISK_ACCOUNT_RISK_RELEASE_WORKER_HPP_
