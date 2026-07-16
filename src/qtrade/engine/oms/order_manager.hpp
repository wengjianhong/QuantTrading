/// @file      order_manager.hpp
/// @brief     订单管理器
/// @details   负责订单的创建、修改、撤销以及状态跟踪
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>
#include <qtrade_sdk/trader/trader_types.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace qtrade::engine::oms {

/// @brief 引擎内订单状态与索引管理
class OrderManager {
 public:
  /// @brief 构造订单管理器
  OrderManager();

  /// @brief 析构订单管理器
  ~OrderManager();

  /// @brief 标记模块为运行中
  void Start();

  /// @brief 标记模块为停止
  void Stop();

  /// @brief 创建订单；同 client_order_id 重复请求返回原订单快照
  /// @param request 下单请求
  /// @return 创建成功返回订单；管理器未启动返回 nullopt
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 在账户预占前分配全局订单 ID
  /// @return 新分配的订单 ID 字符串
  [[nodiscard]] std::string AllocateOrderId();

  /// @brief 使用已预分配的订单 ID 创建 OMS 订单
  /// @param request 下单请求
  /// @param order_id 已分配的全局订单 ID
  /// @return 创建成功返回订单；管理器未启动返回 nullopt
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request,
                                                       const std::string& order_id);

  /// @brief 发送订单（兼容入口）
  /// @param request 下单请求
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode SendOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 撤销订单
  /// @param order_id 全局订单 ID
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode CancelOrder(const std::string& order_id);

  /// @brief 按全局订单 ID 查询
  /// @param order_id 全局订单 ID
  /// @return 存在则返回订单快照
  std::optional<qtrade_sdk::trader::Order> GetOrder(const std::string& order_id) const;

  /// @brief 按客户端订单 ID 查询
  /// @param client_order_id 策略侧客户端订单 ID
  /// @return 存在则返回订单快照
  std::optional<qtrade_sdk::trader::Order> GetOrderByClientId(std::uint32_t client_order_id) const;

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
  /// order_id → 订单
  std::unordered_map<std::string, qtrade_sdk::trader::Order> orders_;
  /// client_order_id → order_id
  std::unordered_map<std::uint32_t, std::string> client_order_index_;
  /// 保护订单表与索引
  mutable std::mutex mutex_;
  /// 是否已 Start
  bool running_ = false;
  /// 订单 ID 递增计数器
  std::atomic<std::uint64_t> order_id_counter_ = 0;
};

}  // namespace qtrade::engine::oms

#endif  // QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_
