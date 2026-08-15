/// @file      compliance_api.hpp
/// @brief     CMS 对引擎内其他模块提供的稳定接口
/// @details   Pipeline 等兄弟模块只依赖本接口，不依赖 StrategyRiskManager。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_CMS_COMPLIANCE_API_HPP_
#define QTRADE_ENGINE_CMS_COMPLIANCE_API_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

namespace qtrade::engine::strategy_risk {

/// @brief CMS 模块间稳定接口（进程内；非 gRPC）
class StrategyRiskApi {
 public:
  virtual ~StrategyRiskApi() = default;

  /// @brief 按 request.strategy_id 检查订单是否满足该策略合规规则
  /// @param request 下单请求（须含非空 strategy_id）
  /// @return 通过返回 kSuccess
  [[nodiscard]] virtual ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const = 0;
};

}  // namespace qtrade::engine::strategy_risk

#endif  // QTRADE_ENGINE_CMS_COMPLIANCE_API_HPP_
