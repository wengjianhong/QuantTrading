/// @file      order_pipeline.hpp
/// @brief     发单准入流水线：合规 → 策略风控 → 实例风控 → OrderIntent 入队
/// @details   A 段在调用线程同步完成；E 段（预占 / OMS / EMS）由 OrderIntentQueue 工作线程执行。
///            Cancel 仍同步写入 OMS 后交给 EMS 撤单队列。
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
#define QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_

#include "qtrade/engine/compliance/compliance_api.hpp"
#include "qtrade/engine/core/order_intent_queue.hpp"
#include "qtrade/engine/execution/execution_api.hpp"
#include "qtrade/engine/instance_risk/instance_risk_api.hpp"
#include "qtrade/engine/orders/order_api.hpp"
#include "qtrade/engine/strategy_risk/strategy_risk_api.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <string>

namespace qtrade::engine {

/// @brief 发单 A 段编排：合规、策略风控、实例风控后将意图入队
class OrderPipeline {
 public:
  /// @brief 构造发单准入流水线
  /// @param compliance 合规检查接口
  /// @param strategy_risk 策略风控接口
  /// @param instance_risk 实例风控接口
  /// @param orders 订单管理接口
  /// @param execution 执行管理接口（撤单入队）
  /// @param intent_queue A 段完成后的意图队列
  OrderPipeline(compliance::ComplianceApi& compliance,
                strategy_risk::StrategyRiskApi& strategy_risk,
                instance_risk::InstanceRiskApi& instance_risk,
                orders::OrderApi& orders,
                execution::ExecutionApi& execution,
                OrderIntentQueue& intent_queue);

  /// @brief 按 A 段准入顺序提交单笔订单
  /// @param request 下单请求
  /// @return A 段拒绝返回对应错误；入队成功返回 kSuccess（不表示已预占或已报出）
  ErrorCode Submit(const qtrade::sdk::trader::OrderRequest& request);

  /// @brief 依次提交订单批次
  /// @param batch 策略订单批次
  /// @return 全部订单入队成功时返回 kSuccess，否则返回首个错误
  ErrorCode SubmitBatch(const qtrade::strategy::OrderBatch& batch);

  /// @brief 撤销指定订单并提交执行队列
  /// @param order_id 全局订单 ID
  /// @return 撤单状态更新或执行入队结果
  ErrorCode Cancel(const std::string& order_id);

 private:
  /// 合规检查接口
  compliance::ComplianceApi& compliance_;
  /// 策略风控接口
  strategy_risk::StrategyRiskApi& strategy_risk_;
  /// 实例风控接口
  instance_risk::InstanceRiskApi& instance_risk_;
  /// 订单管理接口
  orders::OrderApi& orders_;
  /// 执行管理接口
  execution::ExecutionApi& execution_;
  /// OrderIntent 队列
  OrderIntentQueue& intent_queue_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_ORDER_PIPELINE_HPP_
