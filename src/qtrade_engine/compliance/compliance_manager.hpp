/// @file      compliance_manager.hpp
/// @brief     合规规则执行器
/// @details   当前为交易所硬规则的占位执行器；规则数据源接入前不裁决订单。
/// @author    wengjianhong
/// @date      2026-08-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_COMPLIANCE_COMPLIANCE_MANAGER_HPP_
#define QTRADE_ENGINE_COMPLIANCE_COMPLIANCE_MANAGER_HPP_

#include "qtrade/engine/compliance/compliance_api.hpp"

namespace qtrade::engine::compliance {

/// @brief 合规规则执行器
class ComplianceManager final : public ComplianceApi {
 public:
  /// @brief 构造默认合规执行器
  ComplianceManager() = default;

  /// @brief 析构合规执行器
  ~ComplianceManager() override = default;

  /// @brief 执行交易所硬规则
  /// @param request 下单请求
  /// @return 交易所规则数据源接入前始终返回 kSuccess
  [[nodiscard]] ErrorCode CheckOrder(const qtrade::sdk::trader::OrderRequest& request) const override;
};

}  // namespace qtrade::engine::compliance

#endif  // QTRADE_ENGINE_COMPLIANCE_COMPLIANCE_MANAGER_HPP_
