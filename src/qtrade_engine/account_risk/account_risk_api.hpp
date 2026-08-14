/// @file      account_risk_api.hpp
/// @brief     账户硬风控对引擎内其他模块提供的稳定接口
/// @details   Pipeline / EMS / TraderEventHandler 只依赖本接口，不依赖 AccountRiskManager，
///            也不直接持有 IAccountRiskBridge。桥接与身份由组合根注入实现类。
///            与 engine::risk（实例风控）及 qtrade::account_risk（进程外桥接）分层不同。
/// @author    wengjianhong
/// @date      2026-08-14
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ACCOUNT_RISK_ACCOUNT_RISK_API_HPP_
#define QTRADE_ENGINE_ACCOUNT_RISK_ACCOUNT_RISK_API_HPP_

#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <string>

namespace qtrade::engine::account_risk {

/// @brief 账户硬风控模块间稳定接口（进程内；非 gRPC）
class AccountRiskApi {
 public:
  virtual ~AccountRiskApi() = default;

  /// @brief 发单路径同步预占；未注入桥时跳过并返回 kSuccess
  /// @param request 下单请求
  /// @param order_id 已分配的全局订单 ID
  /// @return 预占成功或无需预占返回 kSuccess
  [[nodiscard]] virtual ErrorCode Reserve(const qtrade::sdk::trader::OrderRequest& request,
                                          const std::string& order_id) = 0;

  /// @brief 终态或发送失败时释放预占（实现类异步执行，不阻塞调用方）
  /// @param order_id 全局订单 ID
  /// @param reason 释放原因
  virtual void Release(std::string order_id, qtrade::account_risk::ReleaseReason reason) = 0;
};

}  // namespace qtrade::engine::account_risk

#endif  // QTRADE_ENGINE_ACCOUNT_RISK_ACCOUNT_RISK_API_HPP_
