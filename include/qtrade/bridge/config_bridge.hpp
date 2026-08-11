/// @file      config_bridge.hpp
/// @brief     配置桥接接口与引擎运行配置结构
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_CONFIG_BRIDGE_HPP_
#define QTRADE_BRIDGE_CONFIG_BRIDGE_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/strategy/strategy.hpp>
#include <qtrade/structs/result.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace qtrade::config {

/// @brief 实例级弱一致风险预算
struct RiskBudget {
  /// 配置中心单调递增版本
  std::uint64_t version = 0;
  /// 最大名义价值
  double max_notional = 0.0;
  /// 最大保证金
  double max_margin = 0.0;
  /// 最大开仓订单数
  std::uint64_t max_open_orders = 0;
  /// 安全缓冲
  double safety_buffer = 0.0;
};

/// @brief 引擎实例运行配置
struct EngineConfig {
  /// 引擎实例标识
  std::string engine_id;
  /// 租户标识；须与引导配置一致
  std::string tenant_id;
  /// 交易账户引用（不含凭证）
  std::string account_id;
  /// 主行情源
  std::string quote_source;
  /// 策略绑定列表
  std::vector<qtrade::strategy::StrategyConfig> strategies;
  /// 备用行情源；空表示不启用自动切换
  std::string quote_failover;
  /// 实例级弱一致风险预算
  RiskBudget risk_budget;
  /// 配置失效时间（Unix 毫秒）；0 表示由服务端默认 TTL 决定
  std::int64_t valid_until_unix_ms = 0;
  /// 交易适配器类型：mock 或 emt
  std::string execution_adapter;
  /// 主行情源连接串
  std::string quote_connection_string;
  /// 配置中心单调递增版本
  std::uint64_t version = 0;
};

/// @brief 配置桥接器
/// @details 注入引擎前须已可用（启动时拉取一次运行配置）；不支持运行时推送热更。
///          连接等生命周期由实现方 / 持有方管理，本接口不包含 Start/Stop。
class IConfigBridge {
 public:
  virtual ~IConfigBridge() = default;

  /// @brief 读取当前引擎运行配置（启动时拉取并缓存）
  /// @return Result<EngineConfig> 当前配置
  virtual Result<EngineConfig> GetEngineConfig() const = 0;
};

}  // namespace qtrade::config

#endif  // QTRADE_BRIDGE_CONFIG_BRIDGE_HPP_
