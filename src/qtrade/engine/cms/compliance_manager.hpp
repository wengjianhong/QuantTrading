/// @file      compliance_manager.hpp
/// @brief     合规管理器
/// @details   负责交易合规检查，确保交易符合监管要求
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <atomic>

namespace qtrade::engine::cms {

/// 订单合规检查的最小实现。
class ComplianceManager {
 public:
  ComplianceManager() = default;
  ~ComplianceManager() = default;

  void Start();
  void Stop();

  /// 检查订单是否满足基础合规规则。
  [[nodiscard]] ErrorCode CheckOrder(const qtrade_sdk::trader::OrderRequest& request) const;

 private:
  std::atomic_bool running_{false};
};

}  // namespace qtrade::engine::cms

#endif  // QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
