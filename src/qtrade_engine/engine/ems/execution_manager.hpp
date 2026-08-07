/// @file      execution_manager.hpp
/// @brief     交易执行管理器（实现 ExecutionApi）
/// @details   有界队列异步报单/撤单；独立工作线程调用 TraderApi，结果直接回写 OMS；
///            发送失败时尽力调用 account-risk ReleaseOrder。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_

#include "qtrade/engine/ems/execution_api.hpp"

#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade_sdk/trader/trader_api.hpp>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace qtrade::engine::oms {
class OrderApi;
}

namespace qtrade::engine::ems {

/// @brief 引擎内 EMS：有界队列异步报单与撤单
class ExecutionManager final : public ExecutionApi {
 public:
  /// @brief 析构并停止工作线程
  ~ExecutionManager() override;

  /// @brief 启动 EMS 工作线程
  void Start();

  /// @brief 停止工作线程并清空待发送队列
  void Stop();

  /// @brief 绑定交易通道 API
  /// @param trader_api 交易适配器指针；可为 nullptr 表示解除绑定
  void SetTraderApi(qtrade_sdk::trader::TraderApi* trader_api);

  /// @brief 绑定 OMS 稳定接口，用于发送前标记与结果回写
  /// @param order_api OMS 模块间接口；可为 nullptr
  void SetOrderApi(oms::OrderApi* order_api);

  /// @brief 绑定 account-risk 桥接（发送失败时释放预占）
  /// @param account_risk_bridge 桥接；可为 nullptr 表示不释放
  void SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* account_risk_bridge);

  /// @brief 设置 ReleaseOrder 所需的租户/账户身份
  void SetAccountRiskIdentity(std::string tenant_id, std::string account_id);

  /// @brief 将新单加入 EMS 有界队列
  /// @param order OMS 订单快照
  /// @return 成功返回 kSuccess，队列满返回 kResourceExhausted；未启动返回 kNotInitialized
  ErrorCode Enqueue(const qtrade_sdk::trader::Order& order) override;

  /// @brief 将撤单加入 EMS 有界队列（优先于新单）
  /// @param request 撤单请求
  /// @return 成功返回 kSuccess，队列满返回 kResourceExhausted；未启动返回 kNotInitialized
  ErrorCode EnqueueCancel(const qtrade_sdk::trader::CancelOrderRequest& request) override;

 private:
  /// @brief EMS 工作项
  struct WorkItem {
    /// @brief 工作项类型
    enum class Type {
      /// 新单
      kSend,
      /// 撤单
      kCancel,
    };

    /// 工作项类型
    Type type = Type::kSend;
    /// 新单快照
    qtrade_sdk::trader::Order order;
    /// 撤单请求
    qtrade_sdk::trader::CancelOrderRequest cancel;
  };

  /// @brief 工作线程主循环：出队并调用交易通道
  void Run();

  /// @brief 发送失败时尽力释放 account-risk 预占
  void ReleaseReservationOnSendFailure(qtrade::account_risk::IAccountRiskBridge* account_risk_bridge,
                                       const std::string& tenant_id,
                                       const std::string& account_id,
                                       const std::string& order_id);

  /// EMS 队列容量
  static constexpr std::size_t kQueueCapacity = 8192;
  /// 交易通道 API；未设置时入队失败
  qtrade_sdk::trader::TraderApi* trader_api_ = nullptr;
  /// OMS 稳定接口；未设置时跳过状态回写
  oms::OrderApi* order_api_ = nullptr;
  /// account-risk 桥接；未设置时跳过失败释放
  qtrade::account_risk::IAccountRiskBridge* account_risk_bridge_ = nullptr;
  /// ReleaseOrder 租户 ID
  std::string tenant_id_;
  /// ReleaseOrder 账户 ID
  std::string account_id_;
  /// 待发送工作项队列；撤单从队头插入
  std::deque<WorkItem> pending_items_;
  /// 保护队列与运行状态
  std::mutex mutex_;
  /// 工作线程等待/唤醒条件变量
  std::condition_variable cv_;
  /// EMS 工作线程
  std::thread worker_;
  /// 是否已 Start
  bool running_ = false;
};

}  // namespace qtrade::engine::ems

#endif  // QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_
