/// @file      engine_lifecycle.hpp
/// @brief     交易引擎生命周期状态机
/// @details   全部合法迁移由 Transition(target, reason) 表驱动；状态枚举见 engine.hpp。
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_LIFECYCLE_HPP_
#define QTRADE_ENGINE_ENGINE_LIFECYCLE_HPP_

#include <qtrade/engine/engine.hpp>
#include <qtrade/error_code/error_codes.hpp>

#include <atomic>
#include <mutex>
#include <string>

namespace qtrade::engine {

/// @brief 线程安全的引擎生命周期控制器
/// @details 合法边：
///          New → Initiated|Failed|Stopped；
///          Stopped → Initiated；
///          Initiated → Ready|Frozen|Draining|Failed|Stopped；
///          Ready → Frozen|Draining|Failed|Stopped；
///          Frozen → Ready|Draining|Failed|Stopped；
///          Draining → Stopped|Failed；
///          Failed → Stopped。
///          已处于目标态时 Transition 幂等成功，并更新 reason。
class EngineLifecycle {
 public:
  /// @brief 查询当前状态
  /// @return 当前生命周期状态
  [[nodiscard]] EngineState State() const;

  /// @brief 获取状态描述
  /// @param state 状态
  /// @return 状态描述
  [[nodiscard]] const std::string& GetStateDescription(EngineState state) const;

  /// @brief 读取最近迁移原因
  /// @return 原因字符串副本；无记录时为空
  [[nodiscard]] std::string Reason() const;

  /// @brief 查询是否允许新单
  /// @return 仅状态为 kReady 时返回 true
  [[nodiscard]] bool IsReady() const;

  /// @brief 表驱动状态迁移（统一入口）
  /// @param target 目标状态
  /// @param reason 迁移原因；空字符串表示清除已有原因
  /// @return 合法或已在目标态返回 kSuccess；非法或 CAS 失败返回 kInvalidState
  ErrorCode Transition(EngineState target, std::string reason = {});

 private:
  /// 当前状态
  std::atomic<EngineState> state_ = EngineState::kNew;
  /// 保护原因字符串
  mutable std::mutex mutex_;
  /// 最近迁移原因
  std::string reason_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_LIFECYCLE_HPP_
