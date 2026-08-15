/// @file      engine.hpp
/// @brief     交易引擎对外稳定抽象（门面）与运行配置域模型
/// @details   客户与官方进程入口应只依赖本头文件中的 IEngine / CreateEngine / EngineConfig，
///            而不直接依赖 TradingEngine 内部模块（OMS/EMS/EventLanes 等）。
///
///            推荐调用顺序：
///              CreateEngine()
///                → 就绪账户/风控桥接 → SetAccount*Bridge（可选，须在 Init 前）
///                → 就绪行情/交易适配器 → Set*Api（须在 Start 前；建议 Init 前注入）
///                → Init(EngineConfig)（启动时注入一次运行配置）
///                → 对每个策略 AddStrategy(config, plugin_so_path)
///                → Start() / Stop()（整引擎启停；不支持运行中单策略启停或热更配置）
///                → … 运行 …
///                → Stop()
///                → 持有方再 Shutdown / 销毁桥接
///
///            本地引导 JSON（日志、服务端点、插件目录）由进程入口私有持有，不进入本接口。
///            桥接指针由调用方持有，须保证活到 Stop 完成之后。
///            注入的 I*Bridge / QuoteApi / TraderApi 须已可用；引擎不创建厂商适配器。
///
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_HPP_
#define QTRADE_ENGINE_ENGINE_HPP_

#include <qtrade/bridge/account_bridge.hpp>
#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/quote/quote_api.hpp>
#include <qtrade/sdk/trader/trader_api.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace qtrade::engine {

/// @brief 引擎运行配置（经 Init 注入；不含策略绑定、风控与适配器选型）
/// @details 策略由 AddStrategy 登记（含策略级风控）；行情/交易适配器由 SetQuoteApi / SetTraderApi 注入。
///          账户硬风控由 IAccountRiskBridge 裁决。本地引导配置不进入本结构。
struct EngineConfig {
  /// 引擎实例标识
  std::string engine_id;
  /// 交易账户引用（不含凭证）
  std::string account_id;
  /// 主行情源（观测/日志）
  std::string quote_source;
  /// 备用行情源；空表示不启用自动切换
  std::string quote_failover;
  /// 最后一笔有效 Tick 允许的最大静默时间（毫秒）；须 > 0
  std::int32_t quote_max_stale_ms = 3000;
};

/// @brief 引擎生命周期状态（IEngine::State 与 EngineLifecycle 共用）
/// @details 合法迁移见 EngineLifecycle。
enum class EngineState {
  /// 尚未初始化（CreateEngine 之后）
  kNew = 0,
  /// Init 已完成，可登记策略并调用 Start
  kInitiated,
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

/// @brief 交易引擎对外抽象接口
class IEngine {
 public:
  virtual ~IEngine() = default;

  // ---------------------------------------------------------------------------
  // 生命周期
  // ---------------------------------------------------------------------------

  /// @brief 初始化引擎
  /// @param config 引擎运行配置（身份与行情源等）；须含非空 engine_id / account_id
  /// @return ErrorCode::kSuccess 表示成功
  virtual ErrorCode Init(const EngineConfig& config) = 0;

  /// @brief 启动引擎
  /// @return ErrorCode::kSuccess 表示成功
  virtual ErrorCode Start() = 0;

  /// @brief 停止引擎
  /// @details Stop 完成内部对象销毁，返回后可销毁本对象
  /// @return 已运行并完成停机返回 kSuccess；未处于运行态时可能返回错误码
  virtual ErrorCode Stop() = 0;

  // ---------------------------------------------------------------------------
  // 状态查询
  // ---------------------------------------------------------------------------

  /// @brief 当前生命周期状态（与内部状态机同一枚举）
  [[nodiscard]] virtual EngineState State() const = 0;

  /// @brief 是否已 Start 且尚未 Stop
  [[nodiscard]] virtual bool IsRunning() const = 0;

  // ---------------------------------------------------------------------------
  // 依赖注入（桥接须在 Init 前；适配器须在 Start 前）
  // ---------------------------------------------------------------------------

  /// @brief 注入账户桥接；须已可用（可选）
  /// @param bridge 非拥有指针；生命周期与就绪状态由调用方管理
  virtual void SetAccountBridge(qtrade::account::IAccountBridge* bridge) = 0;

  /// @brief 注入账户硬风控桥接；须已可用（可选；非空则启用 E 段）
  /// @param bridge 非拥有指针；生命周期与就绪状态由调用方管理
  virtual void SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* bridge) = 0;

  /// @brief 注入行情适配器；
  /// @details 调用方保证在注入前完成配置与 Connect，引擎在 Start 时仅校验连接状态，不负责 Connect
  /// @param quote_api 所有权转入引擎；调用方须在注入前完成配置与 Connect，可在 Init 前后设置但须在 Start 前
  virtual void SetQuoteApi(std::unique_ptr<qtrade::sdk::quote::QuoteApi> quote_api) = 0;

  /// @brief 注入交易适配器；
  /// @details 调用方保证在注入前完成配置与 Connect，引擎在 Start 时仅校验连接状态，不负责 Connect
  /// @param trader_api 所有权转入引擎；调用方须在注入前完成配置与 Connect，可在 Init 前后设置但须在 Start 前
  virtual void SetTraderApi(std::unique_ptr<qtrade::sdk::trader::TraderApi> trader_api) = 0;

  // ---------------------------------------------------------------------------
  // 策略登记（须在 Start 之前；不支持运行中增删或单策略启停）
  // ---------------------------------------------------------------------------

  /// @brief 按 .so 路径加载插件并登记策略实例
  /// @details 引擎校验 plugin_so_path 为已存在的常规文件后 dlopen，按 config.strategy_name
  ///          匹配 ABI 插件名创建实例，注入发单回调并 Init(config)；同时按 config 登记行情订阅与 CMS 规则。
  ///          plugin_so_path 为部署侧本地路径，不进 StrategyConfig / proto；不可为空。
  /// @param config 策略实例配置（strategy_id / instruments / risk 等）
  /// @param plugin_so_path 策略插件 .so 完整路径；文件不存在返回 kNotSuchFileOrDirectory
  /// @return ErrorCode::kSuccess 表示成功（disabled 策略直接跳过亦返回成功）
  virtual ErrorCode AddStrategy(const qtrade::strategy::StrategyConfig& config, const std::string& plugin_so_path) = 0;
};

/// @brief 创建引擎实例（隐藏具体实现类型）
/// @return 非空 unique_ptr；调用方按 IEngine 契约使用
[[nodiscard]] std::unique_ptr<IEngine> CreateEngine();

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_HPP_
