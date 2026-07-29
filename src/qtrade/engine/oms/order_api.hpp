/// @file      order_api.hpp
/// @brief     OMS 对引擎内其他模块提供的稳定接口
/// @details   Pipeline / EMS 等兄弟模块只依赖本接口，不依赖 OrderManager。
///            生命周期初始化、柜台对账等由组合根通过 OrderManager 调用。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_OMS_ORDER_API_HPP_
#define QTRADE_ENGINE_OMS_ORDER_API_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace qtrade::engine::oms {

/// @brief 引擎内订单生命周期状态
enum class OrderLifecycleState : std::uint8_t {
  /// 已完成本地准入
  kPrepared = 0,
  /// 已进入 EMS 队列
  kEmsQueued = 1,
  /// 正在调用交易通道发送
  kSendPending = 2,
  /// 通道已接受，等待后续回报
  kWorking = 3,
  /// 已部分成交
  kPartiallyFilled = 4,
  /// 已全部成交
  kFilled = 5,
  /// 撤单请求已提交
  kCancelPending = 6,
  /// 已撤单
  kCanceled = 7,
  /// 已拒绝或确定发送失败
  kRejected = 8,
  /// 发送结果未知，须查询柜台
  kSendUnknown = 9,
};

/// @brief OMS 模块间稳定接口（进程内；非 gRPC）
class OrderApi {
 public:
  virtual ~OrderApi() = default;

  /// @brief 在账户预占前分配全局订单 ID
  /// @return 新分配的订单 ID 字符串
  [[nodiscard]] virtual std::string AllocateOrderId() = 0;

  /// @brief 使用已预分配的订单 ID 创建 OMS 订单
  /// @param request 下单请求
  /// @param order_id 已分配的全局订单 ID
  /// @return 创建成功返回订单；未 Initialize 返回 nullopt
  virtual std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request,
                                                               const std::string& order_id) = 0;

  /// @brief 按客户端订单 ID 查询
  /// @param client_order_id 策略侧客户端订单 ID
  /// @return 存在则返回订单快照
  [[nodiscard]] virtual std::optional<qtrade_sdk::trader::Order> GetOrderByClientId(
    std::uint32_t client_order_id) const = 0;

  /// @brief 查询订单生命周期状态
  /// @param order_id 全局订单 ID
  /// @return 订单存在时返回生命周期状态
  [[nodiscard]] virtual std::optional<OrderLifecycleState> GetLifecycleState(const std::string& order_id) const = 0;

  /// @brief 记录订单已进入 EMS 队列
  /// @param order_id 全局订单 ID
  /// @return 成功返回 kSuccess
  virtual ErrorCode MarkEmsQueued(const std::string& order_id) = 0;

  /// @brief 记录订单开始调用交易通道
  /// @param order_id 全局订单 ID
  /// @return 成功返回 kSuccess
  virtual ErrorCode MarkSendPending(const std::string& order_id) = 0;

  /// @brief 记录交易通道发送结果
  /// @param order_id 全局订单 ID
  /// @param result 交易通道返回码；kTimeout 进入 SendUnknown
  /// @return 状态更新结果
  virtual ErrorCode RecordSendResult(const std::string& order_id, ErrorCode result) = 0;

  /// @brief 记录撤单调用结果
  /// @param order_id 全局订单 ID
  /// @param result 交易通道返回码
  /// @return 状态更新结果
  virtual ErrorCode RecordCancelResult(const std::string& order_id, ErrorCode result) = 0;
};

}  // namespace qtrade::engine::oms

#endif  // QTRADE_ENGINE_OMS_ORDER_API_HPP_
