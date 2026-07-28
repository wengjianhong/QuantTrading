/// @file      engine_lifecycle.cpp
/// @brief     交易引擎生命周期状态机实现
/// @details   实现启动链相邻迁移校验与 Freeze/Resume/Fail/Drain/Stopped 强制跃迁
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/core/engine_lifecycle.hpp"

namespace qtrade::engine {
namespace {

/// @brief 判断启动阶段是否为相邻迁移
/// @param from 当前状态
/// @param to 目标状态
/// @return 合法返回 true
bool IsSequentialStartupTransition(EngineLifecycleState from, EngineLifecycleState to) {
  // 启动链只允许相邻前进一步：New/Stopped → Bootstrap → Fenced → ... → Ready
  switch (from) {
    case EngineLifecycleState::kNew:
    case EngineLifecycleState::kStopped:
      return to == EngineLifecycleState::kBootstrap;
    case EngineLifecycleState::kBootstrap:
      return to == EngineLifecycleState::kInstanceLocked;
    case EngineLifecycleState::kInstanceLocked:
      return to == EngineLifecycleState::kReplayed;
    case EngineLifecycleState::kReplayed:
      return to == EngineLifecycleState::kBrokerSynced;
    case EngineLifecycleState::kBrokerSynced:
      return to == EngineLifecycleState::kRiskSynced;
    case EngineLifecycleState::kRiskSynced:
      return to == EngineLifecycleState::kMarketHealthy;
    case EngineLifecycleState::kMarketHealthy:
      return to == EngineLifecycleState::kReady;
    case EngineLifecycleState::kReady:
    case EngineLifecycleState::kFrozen:
    case EngineLifecycleState::kDraining:
    case EngineLifecycleState::kFailed:
      return false;
  }
  return false;
}

}  // namespace

EngineLifecycleState EngineLifecycle::State() const {
  return state_.load(std::memory_order_acquire);
}

bool EngineLifecycle::IsReady() const {
  return State() == EngineLifecycleState::kReady;
}

ErrorCode EngineLifecycle::Advance(EngineLifecycleState target) {
  // 1. 校验启动链相邻迁移后 CAS 推进
  EngineLifecycleState current = state_.load(std::memory_order_acquire);
  if (!IsSequentialStartupTransition(current, target)) {
    return ErrorCode::kSystemError;
  }
  if (!state_.compare_exchange_strong(current, target, std::memory_order_acq_rel)) {
    return ErrorCode::kSystemError;
  }
  return ErrorCode::kSuccess;
}

void EngineLifecycle::Freeze(const std::string& reason) {
  // 1. 记录原因并强制进入 Frozen
  {
    std::lock_guard lock(mutex_);
    reason_ = reason;
  }
  state_.store(EngineLifecycleState::kFrozen, std::memory_order_release);
}

ErrorCode EngineLifecycle::ResumeReady() {
  // 1. 仅允许 Frozen → Ready，成功后清除原因
  EngineLifecycleState expected = EngineLifecycleState::kFrozen;
  if (!state_.compare_exchange_strong(expected, EngineLifecycleState::kReady, std::memory_order_acq_rel)) {
    return ErrorCode::kSystemError;
  }
  std::lock_guard lock(mutex_);
  reason_.clear();
  return ErrorCode::kSuccess;
}

void EngineLifecycle::Fail(const std::string& reason) {
  // 启动或运行失败：记录原因并进入终态 kFailed
  {
    std::lock_guard lock(mutex_);
    reason_ = reason;
  }
  state_.store(EngineLifecycleState::kFailed, std::memory_order_release);
}

void EngineLifecycle::BeginDrain() {
  // Stop 入口：标记 kDraining，阻止新单进入
  state_.store(EngineLifecycleState::kDraining, std::memory_order_release);
}

void EngineLifecycle::MarkStopped() {
  // 资源释放完毕，回到 kStopped 以便下次 Init
  state_.store(EngineLifecycleState::kStopped, std::memory_order_release);
}

std::string EngineLifecycle::Reason() const {
  std::lock_guard lock(mutex_);
  return reason_;
}

}  // namespace qtrade::engine
