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

class OrderManager {
 public:
  OrderManager();
  ~OrderManager();

  void Start();
  void Stop();

  /// @brief 创建订单；同 client_order_id 重复请求返回原订单快照。
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request);
  /// @brief 在账户预占前分配全局订单 ID。
  [[nodiscard]] std::string AllocateOrderId();
  /// @brief 使用已预分配的订单 ID 创建 OMS 订单。
  std::optional<qtrade_sdk::trader::Order> CreateOrder(const qtrade_sdk::trader::OrderRequest& request,
                                                       const std::string& order_id);
  ErrorCode SendOrder(const qtrade_sdk::trader::OrderRequest& request);
  ErrorCode CancelOrder(const std::string& order_id);

  std::optional<qtrade_sdk::trader::Order> GetOrder(const std::string& order_id) const;
  std::optional<qtrade_sdk::trader::Order> GetOrderByClientId(std::uint32_t client_order_id) const;
  void UpdateOrderStatus(const std::string& order_id, qtrade_sdk::trader::OrderStatusType status);
  void ApplyOrderReport(const qtrade_sdk::trader::Order& report);
  void ApplyTradeReport(const qtrade_sdk::trader::Trade& report);

 private:
  std::unordered_map<std::string, qtrade_sdk::trader::Order> orders_;
  std::unordered_map<std::uint32_t, std::string> client_order_index_;
  mutable std::mutex mutex_;
  bool running_;
  std::atomic<uint64_t> order_id_counter_;
};

}  // namespace qtrade::engine::oms

#endif  // QTRADE_TRADING_ENGINE_ORDER_MANAGER_HPP_
