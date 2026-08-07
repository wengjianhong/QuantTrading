/// @file      engine.hpp
/// @brief     交易引擎对外稳定抽象（门面）
/// @details   客户与官方进程入口应只依赖本头文件中的 IEngine / CreateEngine，
///            而不直接依赖 TradingEngine 内部模块（OMS/EMS/EventLanes 等）。
///
///            推荐调用顺序：
///              CreateEngine()
///                → 就绪桥接（持有方 Init）→ Set*Bridge（须在 Init 前）
///                → Init(bootstrap)
///                → 就绪行情/交易适配器（持有方创建并 Connect）→ Set*Api（须在 Start 前）
///                → AddStrategy(...) 和/或 LoadStrategiesFromPlugins(plugin_dir)
///                → Start()
///                → … 运行 …
///                → Stop()
///                → 持有方再 Shutdown / 销毁桥接
///
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
#include <qtrade/bridge/config_bridge.hpp>
#include <qtrade/common/config/qtrade_engine_bootstrap_config.hpp>
#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/strategy/strategy.hpp>
#include <qtrade/sdk/quote/quote_api.hpp>
#include <qtrade/sdk/trader/trader_api.hpp>

#include <memory>
#include <string>

namespace qtrade::engine {

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
/// @details 只暴露组合与生命周期；策略经 AddStrategy 进程内注册，
///          或经 LoadStrategiesFromPlugins(plugin_dir) 从指定目录加载（可选）。
class IEngine {
 public:
  virtual ~IEngine() = default;

  // ---------------------------------------------------------------------------
  // 依赖注入（须在 Init 之前调用）
  // ---------------------------------------------------------------------------

  /// @brief 注入配置桥接；须已可用。可为空表示不使用远端配置（须同时关闭 bootstrap 中对应 enabled）
  /// @param bridge 非拥有指针；生命周期与就绪状态由调用方管理
  virtual void SetConfigBridge(qtrade::config::IConfigBridge* bridge) = 0;

  /// @brief 注入账户桥接；须已可用
  /// @param bridge 非拥有指针；生命周期与就绪状态由调用方管理
  virtual void SetAccountBridge(qtrade::account::IAccountBridge* bridge) = 0;

  /// @brief 注入账户硬风控桥接；须已可用
  /// @param bridge 非拥有指针；生命周期与就绪状态由调用方管理
  virtual void SetAccountRiskBridge(qtrade::account_risk::IAccountRiskBridge* bridge) = 0;

  /// @brief 注入行情适配器；须已 Connect。可在 Init 前后设置，但须在 Start 之前
  /// @param quote_api 所有权转入引擎
  virtual void SetQuoteApi(std::unique_ptr<qtrade_sdk::quote::QuoteApi> quote_api) = 0;

  /// @brief 注入交易适配器；须已 Connect。可在 Init 前后设置，但须在 Start 之前
  /// @param trader_api 所有权转入引擎
  virtual void SetTraderApi(std::unique_ptr<qtrade_sdk::trader::TraderApi> trader_api) = 0;

  // ---------------------------------------------------------------------------
  // 策略登记（须在 Start 之前；不支持运行中增删）
  // ---------------------------------------------------------------------------

  /// @brief 进程内注册策略实例（推荐的无 so 接入方式）
  /// @details 引擎负责注入发单回调并调用 strategy->Init(config)，再纳入策略管理器。
  /// @param config 策略实例配置（strategy_id / instruments 等）
  /// @param strategy 策略实例所有权；不可为空
  /// @return ErrorCode::kSuccess 表示成功
  virtual ErrorCode AddStrategy(const qtrade::strategy::StrategyConfig& config,
                                std::unique_ptr<qtrade::strategy::IStrategy> strategy) = 0;

  /// @brief 从指定目录批量加载策略插件，并按当前运行配置中的策略列表创建实例
  /// @param plugin_dir 策略 .so 所在目录（由调用方显式传入，不从 bootstrap 隐式读取）
  /// @return ErrorCode::kSuccess 表示成功
  virtual ErrorCode LoadStrategiesFromPlugins(const std::string& plugin_dir) = 0;

  // ---------------------------------------------------------------------------
  // 生命周期
  // ---------------------------------------------------------------------------

  /// @brief 初始化引擎：校验/启动桥接、拉取运行配置、装配内部模块与默认适配器
  /// @param bootstrap 进程引导配置（日志、身份、支撑服务端点等）
  /// @return ErrorCode::kSuccess 表示成功
  virtual ErrorCode Init(const qtrade::common::config::QtradeEngineBootstrapConfig& bootstrap) = 0;

  /// @brief 启动引擎：连接柜台、对账、事件泵、策略与 EMS
  /// @return ErrorCode::kSuccess 表示成功
  virtual ErrorCode Start() = 0;

  /// @brief 停止引擎：停策略与通道；返回后可销毁本对象
  /// @return 已运行并完成停机返回 kSuccess；未处于运行态时可能返回错误码
  virtual ErrorCode Stop() = 0;

  // ---------------------------------------------------------------------------
  // 状态查询
  // ---------------------------------------------------------------------------

  /// @brief 当前生命周期状态（与内部状态机同一枚举）
  [[nodiscard]] virtual EngineState State() const = 0;

  /// @brief 是否已 Start 且尚未 Stop
  [[nodiscard]] virtual bool IsRunning() const = 0;
};

/// @brief 创建引擎实例（隐藏具体实现类型）
/// @return 非空 unique_ptr；调用方按 IEngine 契约使用
[[nodiscard]] std::unique_ptr<IEngine> CreateEngine();

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_HPP_
