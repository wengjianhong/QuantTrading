/// @file      engine_lifecycle.hpp
/// @brief     交易引擎生命周期状态机
/// @details   约束启动顺序（Bootstrap→…→Ready）与运行期冻结/排空/失败迁移；
///            状态以 atomic 发布，原因字符串由互斥保护
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_LIFECYCLE_HPP_
#define QTRADE_ENGINE_ENGINE_LIFECYCLE_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <atomic>
#include <mutex>
#include <string>

namespace qtrade::engine {

/// @brief 引擎进程生命周期状态
enum class EngineLifecycleState {
  /// 尚未初始化
  kNew = 0,
  /// 正在加载引导配置与依赖
  kBootstrap,
  /// 引擎内模块已就绪（OMS/接线等）
  kModulesReady,
  /// 柜台订单与成交已对账
  kBrokerSynced,
  /// 账户预占已对账
  kRiskSynced,
  /// 行情已通过健康门禁
  kMarketHealthy,
  /// 可接受新单
  kReady,
  /// 已冻结新单，仍处理回报
  kFrozen,
  /// 正在排空并停止
  kDraining,
  /// 已停止
  kStopped,
  /// 启动或运行失败
  kFailed,
};

/// @brief 线程安全的引擎生命周期控制器
/// @details Advance 仅允许启动链上的相邻迁移；Freeze/Fail/BeginDrain/MarkStopped
///          为强制跃迁，不校验来源状态
class EngineLifecycle {
 public:
  /// @brief 查询当前状态
  /// @return 当前生命周期状态
  [[nodiscard]] EngineLifecycleState State() const;

  /// @brief 查询是否允许新单
  /// @return 仅状态为 kReady 时返回 true
  [[nodiscard]] bool IsReady() const;

  /// @brief 按启动顺序推进状态
  /// @param target 目标状态（须为当前状态的合法后继）
  /// @return 合法迁移返回 kSuccess；非法或 CAS 失败返回 kSystemError
  ErrorCode Advance(EngineLifecycleState target);

  /// @brief 冻结新单（仍可处理回报）
  /// @param reason 冻结原因码或说明
  void Freeze(const std::string& reason);

  /// @brief 从可恢复冻结恢复为 READY
  /// @return 当前为 kFrozen 时清除原因并返回 kSuccess；否则返回 kSystemError
  ErrorCode ResumeReady();

  /// @brief 标记启动或运行失败
  /// @param reason 失败原因码或说明
  void Fail(const std::string& reason);

  /// @brief 进入排空状态，停止接受新单并准备关停
  void BeginDrain();

  /// @brief 标记进程已停止
  void MarkStopped();

  /// @brief 读取最近冻结或失败原因
  /// @return 原因字符串副本；无记录时为空
  [[nodiscard]] std::string Reason() const;

 private:
  /// 当前状态
  std::atomic<EngineLifecycleState> state_ = EngineLifecycleState::kNew;
  /// 保护原因字符串
  mutable std::mutex mutex_;
  /// 最近冻结或失败原因
  std::string reason_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_LIFECYCLE_HPP_
