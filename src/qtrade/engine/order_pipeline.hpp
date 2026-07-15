/// @file      order_pipeline.hpp
/// @brief     发单准入流水线：CMS → Risk → E 段预占 → OMS → EMS
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_

#include "qtrade/engine/cms/compliance_manager.hpp"
#include "qtrade/engine/ems/execution_manager.hpp"
#include "qtrade/engine/oms/order_manager.hpp"
#include "qtrade/engine/risk/risk_manager.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <memory>

namespace qtrade::client {
class AccountRiskClient;
class LogClient;
}  // namespace qtrade::client

namespace qtrade::engine {

/// @brief A 段后准入编排：合规、实例风控、账户预占、OMS 落单与 EMS 报送
class OrderPipeline {
 public:
  /// @brief 构造发单流水线
  /// @param compliance 合规模块
  /// @param risk_manager 实例级风控模块
  /// @param order_manager 订单管理模块
  /// @param execution_manager 执行管理模块
  /// @param account_risk_client 账户硬风控客户端；可为 nullptr 表示禁用 E 段
  OrderPipeline(cms::ComplianceManager& compliance,
                risk::RiskManager& risk_manager,
                oms::OrderManager& order_manager,
                ems::ExecutionManager& execution_manager,
                qtrade::client::AccountRiskClient* account_risk_client = nullptr);

  /// @brief 设置或替换账户硬风控客户端
  /// @param account_risk_client 客户端指针；可为 nullptr
  void SetAccountRiskClient(qtrade::client::AccountRiskClient* account_risk_client);

  /// @brief 设置日志客户端，用于 P0 审计门禁查询
  /// @param log_client 日志客户端；可为 nullptr
  void SetLogClient(qtrade::client::LogClient* log_client);

  /// @brief 提交策略订单请求并走完整准入链路
  /// @param request 策略下单请求
  /// @return ErrorCode::kSuccess 表示成功进入 EMS；失败返回对应错误码
  ErrorCode Submit(const qtrade_sdk::trader::OrderRequest& request);

 private:
  /// 合规模块引用
  cms::ComplianceManager& compliance_;
  /// 实例级风控模块引用
  risk::RiskManager& risk_manager_;
  /// 订单管理模块引用
  oms::OrderManager& order_manager_;
  /// 执行管理模块引用
  ems::ExecutionManager& execution_manager_;
  /// 账户硬风控客户端；未启用时为空
  qtrade::client::AccountRiskClient* account_risk_client_ = nullptr;
  /// 日志客户端；用于审计门禁
  qtrade::client::LogClient* log_client_ = nullptr;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
