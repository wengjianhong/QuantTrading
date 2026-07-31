/// @file      order_pipeline.hpp
/// @brief     发单准入流水线：CMS → Risk → E 段预占 → OMS → EMS
/// @details   编排策略订单的本地准入与落单：合规、实例风控、可选账户
///            预占、OMS 持久化与 EMS 入队；入队失败时直接调用 ReleaseOrder。
///            只依赖各模块 XxxApi。
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_

#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/engine/cms/compliance_api.hpp"
#include "qtrade/engine/ems/execution_api.hpp"
#include "qtrade/engine/oms/order_api.hpp"
#include "qtrade/engine/risk/risk_api.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/account_risk/v1/account_risk.pb.h>
#include <qtrade/strategy/strategy.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <string>

namespace qtrade::engine {

/// @brief A 段后准入编排：合规、实例风控、账户预占、OMS 落单与 EMS 报送
class OrderPipeline {
 public:
  /// @brief 构造发单流水线
  OrderPipeline(cms::ComplianceApi& compliance,
                risk::RiskApi& risk,
                oms::OrderApi& orders,
                ems::ExecutionApi& execution,
                qtrade::client::AccountRiskClient* account_risk_client = nullptr);

  /// @brief 设置或替换账户硬风控客户端
  void SetAccountRiskClient(qtrade::client::AccountRiskClient* account_risk_client);

  /// @brief 设置 E 段 RPC 所需的账户身份（组装 Reserve/Release 请求）
  void SetAccountRiskIdentity(std::string tenant_id, std::string account_id, std::string engine_id);

  /// @brief 提交策略订单请求并走完整准入链路
  /// @param request 单笔下单请求
  /// @return 准入成功返回 kSuccess
  ErrorCode Submit(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 提交策略报单批次：按序逐笔 Submit，遇错即停
  /// @param batch 策略产生的报单批次
  /// @return 全部成功返回 kSuccess；否则返回首笔失败错误码
  ErrorCode SubmitBatch(const qtrade::strategy::OrderBatch& batch);

  /// @brief 撤销指定订单（OMS 标记 + EMS 撤单入队）
  /// @param order_id 全局订单 ID
  /// @return 成功进入 EMS 撤单队列返回 kSuccess；订单不存在返回 kNotFound
  ErrorCode Cancel(const std::string& order_id);

 private:
  /// @brief 尽力调用 ReleaseOrder（失败仅影响预占回收，不改变本函数主路径返回值）
  void ReleaseReservation(const std::string& order_id,
                          qtrade::account_risk::v1::ReleaseOrderRequest::Reason reason);

  cms::ComplianceApi& compliance_;
  risk::RiskApi& risk_;
  oms::OrderApi& orders_;
  ems::ExecutionApi& execution_;
  qtrade::client::AccountRiskClient* account_risk_client_ = nullptr;
  std::string tenant_id_;
  std::string account_id_;
  std::string engine_id_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
