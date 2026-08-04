/// @file      order_manager.hpp
/// @brief     订单管理器（进程内内存状态机；实现 OrderApi）
/// @details   职责分层：
///            1) OrderApi：Pipeline / EMS 等模块间稳定接口；
///            2) 本地内存：订单表、索引、幂等与对账标记；
///            3) 业务状态机：生命周期迁移与敞口聚合；
///            4) 柜台对接：委托/成交回报应用与启动期对账 Adopt。
///            不落盘；崩溃恢复以柜台快照为准，不回放本地订单、不按旧意图补单。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_

#include "qtrade/engine/oms/order_api.hpp"

#include <qtrade_sdk/trader/trader_types.hpp>

#include <atomic>
#include <mutex>
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
  /// 引擎 epoch（写入全局 order_id）
  std::uint64_t engine_epoch = 1;
};

/// @brief 引擎内订单状态与索引管理（仅内存）
class OrderManager final : public OrderApi {
 public:
  /// @brief 构造订单管理器
  OrderManager();

  /// @brief 析构订单管理器
  ~OrderManager() override;

  // ---------------------------------------------------------------------------
  // 生命周期（组合根）
  // ---------------------------------------------------------------------------

  /// @brief 初始化内存 OMS（清空表并绑定身份）
  /// @param options OMS 初始化选项
  /// @return 成功返回 kSuccess
  ErrorCode Initialize(const OrderManagerOptions& options);

  /// @brief 清空订单表并标记未初始化
  void Shutdown();

  // ---------------------------------------------------------------------------
  // OrderApi：模块间稳定接口（Pipeline / EMS）
  // ---------------------------------------------------------------------------

  /// @brief 在账户预占前分配全局订单 ID
  /// @return 新分配的订单 ID 字符串
  [[nodiscard]] std::string AllocateOrderId() override;

  /// @brief 使用已预分配的订单 ID 创建 OMS 订单
  /// @param request 下单请求
  /// @param order_id 已分配的全局订单 ID
  /// @return 创建成功返回订单；未 Initialize 返回 nullopt
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request,
                                                       const std::string& order_id) override;

  /// @brief 按客户端订单 ID 查询
  /// @param client_order_id 策略侧客户端订单 ID
  /// @return 存在则返回订单快照
  [[nodiscard]] std::optional<qtrade_sdk::trader::Order> GetOrderByClientId(
    std::uint32_t client_order_id) const override;

  /// @brief 按全局订单 ID 查询
  /// @param order_id 全局订单 ID
  /// @return 存在则返回订单快照
  [[nodiscard]] std::optional<qtrade_sdk::trader::Order> GetOrder(const std::string& order_id) const override;

  /// @brief 查询订单生命周期状态
  /// @param order_id 全局订单 ID
  /// @return 订单存在时返回生命周期状态
  [[nodiscard]] std::optional<OrderLifecycleState> GetLifecycleState(const std::string& order_id) const override;

  /// @brief 撤销订单（本地进入 kCancelPending；真正撤单由 EMS 调通道）
  /// @param order_id 全局订单 ID
  /// @return 成功记录撤单请求并进入 kCancelPending
  ErrorCode CancelOrder(const std::string& order_id) override;

  /// @brief 记录订单已进入 EMS 队列
  /// @param order_id 全局订单 ID
  /// @return 成功返回 kSuccess
  ErrorCode MarkEmsQueued(const std::string& order_id) override;

  /// @brief 记录订单开始调用交易通道
  /// @param order_id 全局订单 ID
  /// @return 成功返回 kSuccess
  ErrorCode MarkSendPending(const std::string& order_id) override;

  /// @brief 记录交易通道发送结果
  /// @param order_id 全局订单 ID
  /// @param result 交易通道返回码；kTimeout 进入 SendUnknown
  /// @return 状态更新结果
  ErrorCode RecordSendResult(const std::string& order_id, ErrorCode result) override;

  /// @brief 记录撤单调用结果
  /// @param order_id 全局订单 ID
  /// @param result 交易通道返回码
  /// @return 状态更新结果
  ErrorCode RecordCancelResult(const std::string& order_id, ErrorCode result) override;

  // ---------------------------------------------------------------------------
  // 本地内存查询与敞口（组合根 / Risk；非 OrderApi）
  // ---------------------------------------------------------------------------

  /// @brief 创建订单；同 client_order_id 重复请求返回原订单快照
  /// @details 便捷入口：内部 AllocateOrderId 后写入本地表（单测等）
  /// @param request 下单请求
  /// @return 创建成功返回订单；未 Initialize 返回 nullopt
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 返回仍需向柜台查询的活动/不确定订单
  /// @return SendPending、SendUnknown、Working、CancelPending 且尚未 MarkReconciled 的订单快照
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

  // ---------------------------------------------------------------------------
  // 柜台对接：回报与对账（组合根 SPI；不直接调 TraderApi）
  // ---------------------------------------------------------------------------

  /// @brief 应用订单回报快照
  /// @param report 订单回报
  void ApplyOrderReport(const qtrade_sdk::trader::Order& report);

  /// @brief 应用成交回报
  /// @param report 成交回报
  void ApplyTradeReport(const qtrade_sdk::trader::Trade& report);

  /// @brief 柜台对账：无本地订单则按快照采纳进内存（不触发发单），有则合并更新
  /// @param report 柜台订单快照
  void ReconcileBrokerOrder(const qtrade_sdk::trader::Order& report);

 private:
  // ---------------------------------------------------------------------------
  // 本地内存数据模型
  // ---------------------------------------------------------------------------

  /// @brief OMS 内部订单条目
  struct OrderEntry {
    /// 订单快照
    qtrade_sdk::trader::Order order;
    /// 引擎内生命周期
    OrderLifecycleState lifecycle_state = OrderLifecycleState::kPrepared;
  };

  /// @brief 按 order_id 或 client_order_id 定位（调用方已持锁）
  /// @param order_id 全局订单 ID；可为空则仅按 client_order_id 查
  /// @param client_order_id 策略侧客户端订单 ID；0 表示不使用
  /// @return 命中则返回迭代器，否则等于 orders_.end()
  [[nodiscard]] std::unordered_map<std::string, OrderEntry>::iterator FindOrderLocked(const std::string& order_id,
                                                                                      std::uint32_t client_order_id);

  /// @copydoc FindOrderLocked
  [[nodiscard]] std::unordered_map<std::string, OrderEntry>::const_iterator FindOrderLocked(
    const std::string& order_id, std::uint32_t client_order_id) const;

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
  std::atomic<std::uint64_t> order_id_counter_{0};
  /// 租户 ID，用于生成全局订单 ID
  std::string tenant_id_;
  /// 引擎 ID，用于生成全局订单 ID
  std::string engine_id_;
  /// 当前引擎 epoch
  std::uint64_t engine_epoch_ = 1;
  /// 是否已 Initialize
  bool initialized_ = false;

  // ---------------------------------------------------------------------------
  // 业务状态机（生命周期迁移）
  // ---------------------------------------------------------------------------

  /// @brief 校验并推进生命周期（仅内存）
  /// @param entry 当前条目
  /// @param target_state 目标生命周期状态
  /// @return 合法迁移返回 kSuccess
  ErrorCode ApplyTransition(OrderEntry& entry, OrderLifecycleState target_state);

  /// @brief 判断状态迁移是否合法
  /// @param from 当前状态
  /// @param to 目标状态
  /// @return 合法返回 true
  [[nodiscard]] static bool CanTransition(OrderLifecycleState from, OrderLifecycleState to);

  // ---------------------------------------------------------------------------
  // 柜台快照合并与状态投影
  // ---------------------------------------------------------------------------

  /// @brief 将柜台/回报快照字段合并进本地条目
  /// @param entry 本地条目
  /// @param report 外部订单快照
  static void MergeOrderSnapshot(OrderEntry& entry, const qtrade_sdk::trader::Order& report);

  /// @brief 由柜台订单状态推导生命周期
  /// @param status 柜台/回报订单状态
  /// @param fallback 无法映射时的回退状态（通常为 Working）
  /// @return 对应生命周期状态
  [[nodiscard]] static OrderLifecycleState LifecycleFromOrderStatus(qtrade_sdk::trader::OrderStatusType status,
                                                                    OrderLifecycleState fallback);
};

}  // namespace qtrade::engine::oms

#endif  // QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_
