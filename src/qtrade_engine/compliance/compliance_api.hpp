/// @file      compliance_api.hpp
/// @brief     合规模块对引擎内其他模块的稳定接口
/// @details   订单流水线等兄弟模块只依赖本接口，不依赖 ComplianceManager。
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_COMPLIANCE_COMPLIANCE_API_HPP_
#define QTRADE_ENGINE_COMPLIANCE_COMPLIANCE_API_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

namespace qtrade::engine::compliance {

/// @brief 合规模块间稳定接口
class ComplianceApi {
 public:
  virtual ~ComplianceApi() = default;

  /// @brief 检查订单是否命中合规规则
  /// @param request 下单请求
  /// @return 通过返回 kSuccess
  [[nodiscard]] virtual ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const = 0;
};

}  // namespace qtrade::engine::compliance

#endif  // QTRADE_ENGINE_COMPLIANCE_COMPLIANCE_API_HPP_
