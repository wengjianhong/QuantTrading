/// @file      execution_manager.hpp
/// @brief     交易执行管理器
/// @details   有界队列异步报单/撤单；独立工作线程调用 TraderApi，结果经回调回写 OMS
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_EXECUTION_MANAGER_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_api.hpp>

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace qtrade::engine::ems {

/// @brief 引擎内 EMS：有界队列异步报单与撤单
class ExecutionManager {
 public:
  /// @brief 发送前持久化回调
  using BeforeSendHandler = std::function<ErrorCode(const std::string& order_id)>;
  /// @brief 交易通道调用结果回调
  using ResultHandler = std::function<void(const std::string& order_id, ErrorCode result)>;

  /// @brief 析构并停止工作线程
  ~ExecutionManager();

  /// @brief 启动 EMS 工作线程
  void Start();

  /// @brief 停止工作线程并清空待发送队列
  void Stop();

  /// @brief 绑定交易通道 API
  /// @param trader_api 交易适配器指针；可为 nullptr 表示解除绑定
  void SetTraderApi(qtrade_sdk::trader::TraderApi* trader_api);

  /// @brief 设置发送前与发送结果回调
  /// @param before_send 调用 TraderApi 前执行；失败时不发送
  /// @param send_result SendOrder 返回后执行
  /// @param cancel_result CancelOrder 返回后执行
  void SetResultHandlers(BeforeSendHandler before_send,
                         ResultHandler send_result,
                         ResultHandler cancel_result);

  /// @brief 将新单加入 EMS 有界队列
  /// @param order OMS 订单快照
  /// @return 成功返回 kSuccess，队列满返回 kResourceExhausted；未启动返回 kNotInitialized
  ErrorCode Enqueue(const qtrade_sdk::trader::Order& order);

  /// @brief 将撤单加入 EMS 有界队列（优先于新单）
  /// @param request 撤单请求
  /// @return 成功返回 kSuccess，队列满返回 kResourceExhausted；未启动返回 kNotInitialized
  ErrorCode EnqueueCancel(const qtrade_sdk::trader::CancelOrderRequest& request);

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

  /// EMS 队列容量
  static constexpr std::size_t kQueueCapacity = 8192;
  /// 交易通道 API；未设置时入队失败
  qtrade_sdk::trader::TraderApi* trader_api_ = nullptr;
  /// 待发送工作项队列；撤单从队头插入
  std::deque<WorkItem> pending_items_;
  /// 发送前持久化回调
  BeforeSendHandler before_send_;
  /// 新单发送结果回调
  ResultHandler send_result_;
  /// 撤单发送结果回调
  ResultHandler cancel_result_;
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
