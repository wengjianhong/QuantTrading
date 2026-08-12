/// @file      risk_manager.hpp
/// @brief     风险管理器
/// @details   校验单笔参数、活动订单数与累计名义敞口是否超出实例风险预算
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0

#ifndef QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_

#include "qtrade/engine/risk/risk_api.hpp"

#include <functional>
#include <limits>
#include <mutex>

namespace qtrade::engine::risk {

/// @brief 实例级风险预算
struct RiskLimits {
  /// 配置版本
  std::uint64_t version = 0;
  /// 单笔最大数量
  std::int64_t max_order_volume = std::numeric_limits<std::int64_t>::max();
  /// 单笔最大名义金额；0 表示不限制
  double max_order_notional = 0.0;
  /// 实例累计最大名义敞口；0 表示不限制
  double max_total_notional = 0.0;
  /// 最大活动订单数；0 表示不限制
  std::uint64_t max_open_orders = 0;
  /// 从名义金额上限中预留的安全缓冲
  double safety_buffer = 0.0;
};

/// @brief 订单参数、活动订单与名义敞口风控
class RiskManager final : public RiskApi {
 public:
  /// @brief 构造默认预算的风险管理器
  RiskManager() = default;

  /// @brief 析构风险管理器
  ~RiskManager() override = default;

  /// @brief 原子替换风险预算
  /// @param limits 新风险预算
  /// @return 版本回退或非法参数返回 kSystemError
  ErrorCode Configure(const RiskLimits& limits);

  /// @brief 设置活动订单数与名义敞口读取器
  /// @param open_orders_provider 活动订单数读取器
  /// @param notional_provider 当前名义敞口读取器
  void SetStateProviders(std::function<std::uint64_t()> open_orders_provider,
                         std::function<double()> notional_provider);

  /// @brief 检查订单参数、单笔和累计预算
  /// @param request 下单请求
  /// @return 通过返回 kSuccess
  [[nodiscard]] ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const override;

  /// @brief 查询当前风险配置版本
  /// @return 风险配置版本
  [[nodiscard]] std::uint64_t Version() const override;

 private:
  /// 保护预算与状态读取器
  mutable std::mutex mutex_;
  /// 当前风险预算
  RiskLimits limits_;
  /// 活动订单数读取器
  std::function<std::uint64_t()> open_orders_provider_;
  /// 当前名义敞口读取器
  std::function<double()> notional_provider_;
};

}  // namespace qtrade::engine::risk

#endif  // QTRADE_TRADING_ENGINE_RISK_MANAGER_HPP_
