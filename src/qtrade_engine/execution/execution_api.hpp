/// @file      execution_api.hpp
/// @brief     EMS 对引擎内其他模块提供的稳定接口
/// @details   Pipeline 等兄弟模块只依赖本接口（入队），不依赖 ExecutionManager。
///            Start/Stop、TraderApi/OMS 接线由组合根通过 ExecutionManager 完成。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_EMS_EXECUTION_API_HPP_
#define QTRADE_ENGINE_EMS_EXECUTION_API_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

namespace qtrade::engine::execution {

/// @brief EMS 模块间稳定接口（进程内；非 gRPC）
class ExecutionApi {
 public:
  /// @brief 析构执行接口
  virtual ~ExecutionApi() = default;

  /// @brief 将新单加入 EMS 有界队列
  /// @param order OMS 订单快照
  /// @return 成功返回 kSuccess，队列满返回 kResourceExhausted；未启动返回 kNotInitialized
  virtual ErrorCode Enqueue(const qtrade::sdk::trader::Order& order) = 0;

  /// @brief 将撤单加入 EMS 有界队列（优先于新单）
  /// @param request 撤单请求
  /// @return 成功返回 kSuccess，队列满返回 kResourceExhausted；未启动返回 kNotInitialized
  virtual ErrorCode EnqueueCancel(const qtrade::sdk::trader::CancelOrderRequest& request) = 0;
};

}  // namespace qtrade::engine::execution

#endif  // QTRADE_ENGINE_EMS_EXECUTION_API_HPP_
