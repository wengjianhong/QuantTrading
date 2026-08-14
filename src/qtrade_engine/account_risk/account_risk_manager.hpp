/// @file      account_risk_manager.hpp
/// @brief     账户硬风控管理器（实现 AccountRiskApi；唯一持有 IAccountRiskBridge）
/// @details   Reserve 同步调用桥接；Release 入队由工作线程调用，避免堵住 Lane-T / EMS。
/// @author    wengjianhong
/// @date      2026-08-14
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ACCOUNT_RISK_ACCOUNT_RISK_MANAGER_HPP_
#define QTRADE_ENGINE_ACCOUNT_RISK_ACCOUNT_RISK_MANAGER_HPP_

#include "qtrade/engine/account_risk/account_risk_api.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace qtrade::engine::account_risk {

/// @brief 引擎内账户硬风控：持有进程外桥接，对兄弟模块只暴露 AccountRiskApi
class AccountRiskManager final : public AccountRiskApi {
 public:
  /// 异步 Release 队列容量；满则阻塞入队方
  static constexpr std::size_t kQueueCapacity = 8192;

  AccountRiskManager();
  ~AccountRiskManager() override;

  AccountRiskManager(const AccountRiskManager&) = delete;
  AccountRiskManager& operator=(const AccountRiskManager&) = delete;

  /// @brief 注入或清除硬风控桥（非拥有）
  /// @param bridge 可空；空则 Reserve 跳过、Release 忽略
  void SetBridge(qtrade::account_risk::IAccountRiskBridge* bridge);

  /// @brief 设置 Reserve/Release 所用身份
  /// @param account_id 交易账户号
  /// @param engine_id 引擎实例标识
  void SetIdentity(std::string account_id, std::string engine_id);

  /// @brief 启动 Release 工作线程
  void Start();

  /// @brief 拒绝新入队，排干已入队 Release 并 join
  void Stop();

  [[nodiscard]] ErrorCode Reserve(const qtrade::sdk::trader::OrderRequest& request,
                                  const std::string& order_id) override;

  void Release(std::string order_id, qtrade::account_risk::ReleaseReason reason) override;

  /// @brief 当前待处理 Release 个数
  [[nodiscard]] std::size_t PendingCount() const;

 private:
  /// 异步释放工作项
  struct ReleaseItem {
    /// 全局订单 ID
    std::string order_id;
    /// 释放原因
    qtrade::account_risk::ReleaseReason reason = qtrade::account_risk::ReleaseReason::kUnspecified;
  };

  /// @brief 工作线程：出队并调用桥接 Release
  void Run();

  /// @brief 调用桥接 Release；失败只打日志
  void InvokeRelease(qtrade::account_risk::IAccountRiskBridge* bridge,
                     const std::string& account_id,
                     const ReleaseItem& item);

  /// 进程外硬风控桥（非拥有；引擎内仅本类持有）
  qtrade::account_risk::IAccountRiskBridge* bridge_ = nullptr;
  /// Reserve/Release 账户号
  std::string account_id_;
  /// Reserve 敞口上的引擎 ID
  std::string engine_id_;
  /// 待释放队列
  std::deque<ReleaseItem> queue_;
  /// 保护桥、身份、队列与运行标志
  mutable std::mutex mutex_;
  /// 队列非空、未满或停止时唤醒
  std::condition_variable cv_;
  /// Release 工作线程
  std::thread worker_;
  /// 是否已 Start
  bool running_ = false;
  /// Stop 后拒绝新入队
  bool stopping_ = false;
};

}  // namespace qtrade::engine::account_risk

#endif  // QTRADE_ENGINE_ACCOUNT_RISK_ACCOUNT_RISK_MANAGER_HPP_
