/// @file      engine_lifecycle.cpp
/// @brief     交易引擎生命周期状态机实现
/// @details   全部迁移经 Transition(target, reason) 表校验并更新原因。
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/engine_lifecycle.hpp"

#include "spdlog/spdlog.h"

#include <utility>

namespace qtrade::engine {
namespace {

/// @brief 判断 from → to 是否为合法边
/// @param from 当前状态
/// @param to 目标状态
/// @return 合法返回 true
bool IsAllowedTransition(EngineLifecycleState from, EngineLifecycleState to) {
  switch (from) {
    case EngineLifecycleState::kNew:
      return to == EngineLifecycleState::kInitiated || to == EngineLifecycleState::kFailed ||
             to == EngineLifecycleState::kStopped;
    case EngineLifecycleState::kStopped:
      return to == EngineLifecycleState::kInitiated;
    case EngineLifecycleState::kInitiated:
      return to == EngineLifecycleState::kReady || to == EngineLifecycleState::kFrozen ||
             to == EngineLifecycleState::kDraining || to == EngineLifecycleState::kFailed ||
             to == EngineLifecycleState::kStopped;
    case EngineLifecycleState::kReady:
      return to == EngineLifecycleState::kFrozen || to == EngineLifecycleState::kDraining ||
             to == EngineLifecycleState::kFailed || to == EngineLifecycleState::kStopped;
    case EngineLifecycleState::kFrozen:
      return to == EngineLifecycleState::kReady || to == EngineLifecycleState::kDraining ||
             to == EngineLifecycleState::kFailed || to == EngineLifecycleState::kStopped;
    case EngineLifecycleState::kDraining:
      return to == EngineLifecycleState::kStopped || to == EngineLifecycleState::kFailed;
    case EngineLifecycleState::kFailed:
      return to == EngineLifecycleState::kStopped;
  }
  return false;
}

}  // namespace

EngineLifecycleState EngineLifecycle::State() const {
  return state_.load(std::memory_order_acquire);
}

const std::string& EngineLifecycle::GetStateDescription(EngineLifecycleState state) const {
  static const std::array<std::string, static_cast<size_t>(EngineLifecycleState::kFailed) + 1> kStateDescriptions = {
    "New", "Initiated", "Ready", "Frozen", "Draining", "Stopped", "Failed"};
  return kStateDescriptions[static_cast<size_t>(state)];
}

bool EngineLifecycle::IsReady() const {
  return State() == EngineLifecycleState::kReady;
}

ErrorCode EngineLifecycle::Transition(EngineLifecycleState target, std::string reason) {
  EngineLifecycleState current = state_.load(std::memory_order_acquire);
  if (current != target) {
    if (!IsAllowedTransition(current, target)) {
      spdlog::error("EngineLifecycle Transition failed, current={}, target={}",
                    GetStateDescription(current),
                    GetStateDescription(target));
      return ErrorCode::kInvalidState;
    }
    if (!state_.compare_exchange_strong(current, target, std::memory_order_acq_rel)) {
      spdlog::error("EngineLifecycle CAS transition failed, current={}, target={}",
                    GetStateDescription(current),
                    GetStateDescription(target));
      return ErrorCode::kInvalidState;
    }
  }

  std::lock_guard lock(mutex_);
  reason_ = std::move(reason);
  return ErrorCode::kSuccess;
}

std::string EngineLifecycle::Reason() const {
  std::lock_guard lock(mutex_);
  return reason_;
}

}  // namespace qtrade::engine
