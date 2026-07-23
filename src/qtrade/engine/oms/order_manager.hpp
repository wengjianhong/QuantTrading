/// @file      order_manager.hpp
/// @brief     订单管理器
/// @details   管理订单生命周期状态机、client_order_id 索引，并以 OrderJournal 持久化
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_

#include "qtrade/engine/oms/order_journal.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>
#include <qtrade_sdk/trader/trader_types.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace qtrade::engine::oms {

/// @brief OMS 初始化选项
struct OrderManagerOptions {
  /// 租户 ID
  std::string tenant_id;
  /// 引擎 ID
  std::string engine_id;
  /// 引擎 epoch
  std::uint64_t engine_epoch = 1;
  /// 订单主日志路径
  std::string journal_path;
  /// 每条记录是否强制 fsync
  bool fsync_on_append = true;
};

/// @brief 引擎内订单状态与索引管理
class OrderManager {
 public:
  /// @brief 构造订单管理器
  OrderManager();

  /// @brief 析构订单管理器
  ~OrderManager();

  /// @brief 初始化订单主日志并回放历史订单
  /// @param options OMS 初始化选项
  /// @return 成功返回 kSuccess
  ErrorCode Initialize(const OrderManagerOptions& options);

  /// @brief 关闭订单主日志并释放资源
  void Shutdown();

  /// @brief 创建订单；同 client_order_id 重复请求返回原订单快照
  /// @param request 下单请求
  /// @return 创建成功返回订单；未 Initialize 返回 nullopt
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 在账户预占前分配全局订单 ID
  /// @return 新分配的订单 ID 字符串
  [[nodiscard]] std::string AllocateOrderId();

  /// @brief 使用已预分配的订单 ID 创建 OMS 订单
  /// @param request 下单请求
  /// @param order_id 已分配的全局订单 ID
  /// @return 创建成功返回订单；未 Initialize 返回 nullopt
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request,
                                                       const std::string& order_id);

  /// @brief 发送订单（兼容入口）
  /// @param request 下单请求
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode SendOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 撤销订单
  /// @param order_id 全局订单 ID
  /// @return 成功记录撤单请求并进入 kCancelPending
  ErrorCode CancelOrder(const std::string& order_id);

  /// @brief 记录订单已进入 EMS 队列
  /// @param order_id 全局订单 ID
  /// @return 成功返回 kSuccess
  ErrorCode MarkEmsQueued(const std::string& order_id);

  /// @brief 记录订单开始调用交易通道
  /// @param order_id 全局订单 ID
  /// @return 成功返回 kSuccess
  ErrorCode MarkSendPending(const std::string& order_id);

  /// @brief 记录交易通道发送结果
  /// @param order_id 全局订单 ID
  /// @param result 交易通道返回码；kTimeout 进入 SendUnknown
  /// @return 日志与状态更新结果
  ErrorCode RecordSendResult(const std::string& order_id, ErrorCode result);

  /// @brief 记录撤单调用结果
  /// @param order_id 全局订单 ID
  /// @param result 交易通道返回码
  /// @return 日志与状态更新结果
  ErrorCode RecordCancelResult(const std::string& order_id, ErrorCode result);

  /// @brief 按全局订单 ID 查询
  /// @param order_id 全局订单 ID
  /// @return 存在则返回订单快照
  std::optional<qtrade_sdk::trader::Order> GetOrder(const std::string& order_id) const;

  /// @brief 按客户端订单 ID 查询
  /// @param client_order_id 策略侧客户端订单 ID
  /// @return 存在则返回订单快照
  std::optional<qtrade_sdk::trader::Order> GetOrderByClientId(std::uint32_t client_order_id) const;

  /// @brief 查询订单生命周期状态
  /// @param order_id 全局订单 ID
  /// @return 订单存在时返回生命周期状态
  [[nodiscard]] std::optional<OrderLifecycleState> GetLifecycleState(const std::string& order_id) const;

  /// @brief 返回恢复后仍需向柜台查询的订单
  /// @return SendPending、SendUnknown、Working、CancelPending 订单快照
  [[nodiscard]] std::vector<qtrade_sdk::trader::Order> GetOrdersRequiringReconciliation() const;

  /// @brief 标记订单已由启动期柜台快照确认
  /// @param order_id 全局订单 ID
  void MarkReconciled(const std::string& order_id);

  /// @brief 查询活动订单数
  /// @return 非 Filled/Canceled/Rejected 的订单数量
  [[nodiscard]] std::uint64_t GetActiveOrderCount() const;

  /// @brief 查询活动订单剩余名义金额
  /// @return 所有活动订单 price × left_volume 之和
  [[nodiscard]] double GetOpenNotional() const;

  /// @brief 更新订单状态
  /// @param order_id 全局订单 ID
  /// @param status 新状态
  void UpdateOrderStatus(const std::string& order_id, qtrade_sdk::trader::OrderStatusType status);

  /// @brief 应用订单回报快照
  /// @param report 订单回报
  void ApplyOrderReport(const qtrade_sdk::trader::Order& report);

  /// @brief 应用成交回报
  /// @param report 成交回报
  void ApplyTradeReport(const qtrade_sdk::trader::Trade& report);

 private:
  /// @brief OMS 内部订单条目
  struct OrderEntry {
    /// 订单快照
    qtrade_sdk::trader::Order order;
    /// 引擎内生命周期
    OrderLifecycleState lifecycle_state = OrderLifecycleState::kPrepared;
  };

  /// @brief 追加状态快照并更新内存
  /// @param entry 当前条目
  /// @param target_state 目标生命周期状态
  /// @param event_type 日志事件类型
  /// @param message 补充信息
  /// @param trade 可选成交回报
  /// @return 追加成功返回 kSuccess
  ErrorCode PersistTransition(OrderEntry& entry,
                              OrderLifecycleState target_state,
                              OrderJournalEventType event_type,
                              const std::string& message = {},
                              const std::optional<qtrade_sdk::trader::Trade>& trade = std::nullopt);

  /// @brief 判断状态迁移是否合法
  /// @param from 当前状态
  /// @param to 目标状态
  /// @return 合法返回 true
  [[nodiscard]] static bool CanTransition(OrderLifecycleState from, OrderLifecycleState to);

  /// order_id → 订单条目
  std::unordered_map<std::string, OrderEntry> orders_;
  /// client_order_id → order_id
  std::unordered_map<std::uint32_t, std::string> client_order_index_;
  /// 已应用成交回报幂等键
  std::unordered_set<std::string> applied_trade_ids_;
  /// 当前进程启动期已由柜台确认的订单
  std::unordered_set<std::string> reconciled_order_ids_;
  /// 保护订单表与索引
  mutable std::mutex mutex_;
  /// 订单 ID 递增计数器
  std::atomic<std::uint64_t> order_id_counter_ = 0;
  /// 租户 ID，用于生成全局订单 ID
  std::string tenant_id_;
  /// 引擎 ID，用于生成全局订单 ID
  std::string engine_id_;
  /// 当前引擎 epoch
  std::uint64_t engine_epoch_ = 1;
  /// 订单主日志
  OrderJournal journal_;
};

}  // namespace qtrade::engine::oms

#endif  // QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_
