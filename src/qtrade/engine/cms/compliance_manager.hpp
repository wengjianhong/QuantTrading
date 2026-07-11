/// @file      compliance_manager.hpp
/// @brief     合规管理器
/// @details   负责交易合规检查，确保交易符合监管要求
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
#define QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_

namespace qtrade::engine::cms {

class ComplianceManager {
 public:
  void Start();
  void Stop();
};

}  // namespace qtrade::engine::cms

#endif  // QTRADE_TRADING_ENGINE_COMPLIANCE_MANAGER_HPP_
