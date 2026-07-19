/// @file      dao_manager.hpp
/// @brief     进程内全部表 DAO 的拥有式管理器（启动时创建，运行期只使用）
/// @details   由各支撑服务 Initialize 持有；按服务调用对应 Ensure*Schemas 建表后注入 Handler
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_DAO_MANAGER_HPP_
#define QTRADE_DAO_DAO_MANAGER_HPP_

#include "qtrade/dao/account_risk_service/account_risk_ledger.hpp"
#include "qtrade/dao/account_risk_service/account_risk_policy.hpp"
#include "qtrade/dao/account_risk_service/order_reservation.hpp"
#include "qtrade/dao/account_service/account_credential.hpp"
#include "qtrade/dao/account_service/trading_account.hpp"
#include "qtrade/dao/config_service/engine/engine_config.hpp"
#include "qtrade/dao/config_service/risk/instance_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/instrument_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/order_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/quote_health_policy.hpp"
#include "qtrade/dao/config_service/risk/strategy_risk_policy.hpp"
#include "qtrade/dao/config_service/risk/tenant_risk_policy.hpp"

#include <tuple>

namespace qtrade::framework::dao {

/// @brief 统一持有全部表 DAO；由服务进程创建并注入
class DaoManager {
 public:
  DaoManager() = default;
  ~DaoManager() = default;
  DaoManager(DaoManager&&) = delete;
  DaoManager(const DaoManager&) = delete;
  DaoManager& operator=(DaoManager&&) = delete;
  DaoManager& operator=(const DaoManager&) = delete;

  /// @brief 按静态 DAO 类型获取已初始化的实例
  /// @tparam DaoT DAO 类型，必须是 DaoManager 支持的表 DAO
  /// @return 对应 DAO 的可写引用
  template <typename DaoT>
  [[nodiscard]] DaoT& Get() {
    return std::get<DaoT>(daos_);
  }

 private:
  /// 所有已注册 DAO 的编译期存储；类型必须唯一，Get<T>() 通过类型定位。
  using DaoStorage = std::tuple<TradingAccount,
                                AccountCredential,
                                EngineConfig,
                                TenantRiskPolicy,
                                InstanceRiskPolicy,
                                StrategyRiskPolicy,
                                InstrumentRiskPolicy,
                                OrderRiskPolicy,
                                QuoteHealthPolicy,
                                AccountRiskPolicy,
                                OrderReservation,
                                AccountRiskLedger>;

  DaoStorage daos_;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_DAO_MANAGER_HPP_
