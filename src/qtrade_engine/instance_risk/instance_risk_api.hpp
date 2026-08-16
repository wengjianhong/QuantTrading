/// @file      instance_risk_api.hpp
/// @brief     实例风控对引擎内其他模块的稳定接口
/// @details   Pipeline 等兄弟模块只依赖本接口，不依赖 InstanceRiskManager。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_RISK_RISK_API_HPP_
#define QTRADE_ENGINE_RISK_RISK_API_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <cstdint>

namespace qtrade::engine::instance_risk {

/// @brief 实例风控模块间稳定接口
class InstanceRiskApi {
 public:
  /// @brief 析构实例风控接口
  virtual ~InstanceRiskApi() = default;

  /// @brief 查询当前风险配置版本
  /// @return 风险配置版本
  [[nodiscard]] virtual std::uint64_t Version() const = 0;

  /// @brief 检查订单参数、单笔和累计预算
  /// @param request 下单请求
  /// @return 通过返回 kSuccess
  [[nodiscard]] virtual ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const = 0;
};

}  // namespace qtrade::engine::instance_risk

#endif  // QTRADE_ENGINE_RISK_RISK_API_HPP_
