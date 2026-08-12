/// @file      strategy.hpp
/// @brief     策略接口定义
/// @details   定义策略接口 IStrategy 以及相关数据结构
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_STRATEGY_STRATEGY_HPP_
#define QTRADE_TRADING_STRATEGY_STRATEGY_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/sdk/quote/quote_struct.hpp>
#include <qtrade/sdk/trader/trader_struct.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace qtrade::strategy {

/// @brief 报单批次结构体
/// @details 策略产生的报单批次
struct OrderBatch {
  /// 报单批次ID
  std::string batch_id;
  /// 报单批次强度（0.0~1.0）
  double strength = 0.0;
  /// 报单批次时间戳(纳秒)
  std::int64_t timestamp_ns = 0;
  /// 触发报单的Tick/Bar行情的时间戳(纳秒)
  std::int64_t trigger_tick_ns = 0;
  /// 报单批次请求列表
  std::vector<qtrade::sdk::trader::OrderRequest> order_requests;
};

/// @brief 策略级风控/合规限额（0 表示该项不限制；是否登记 CMS 由 StrategyConfig.enabled 决定）
struct StrategyRiskLimits {
  /// 单笔最大名义金额；0 表示不限制
  double max_notional = 0.0;
  /// 单笔最大数量；0 表示不限制
  std::int64_t max_volume = 0;
  /// 最大持仓（绝对值）；0 表示不限制
  std::int64_t max_position_volume = 0;
  /// 发单冷却（毫秒）；0 表示不限制
  std::int32_t order_cooldown_ms = 0;
};

/// @brief 策略行为参数（策略逻辑消费；未用字段保持默认 0，策略可不读）
struct StrategyArgs {
  /// 单笔下单量；0 表示由策略自行决定
  std::int64_t order_volume = 0;
  /// 策略计算窗口(K线/Tick样本数量)；0 表示未配置
  std::int32_t window_size = 0;
  /// 下单信号阈值；0 表示未配置
  double order_threshold = 0.0;
  /// 止损比例；0 表示未配置
  double stop_loss_percent = 0.0;
  /// 止盈比例；0 表示未配置
  double take_profit_percent = 0.0;
};

/// @brief 策略实例配置（与 config.v1.StrategyConfig 字段对齐；不依赖 protobuf）
struct StrategyConfig {
  /// 是否启用
  bool enabled = false;
  /// 策略实例标识
  std::string strategy_id;
  /// 策略插件名
  std::string strategy_name;
  /// 订阅合约列表
  std::vector<std::string> instruments;
  /// 策略行为参数
  StrategyArgs args;
  /// 策略级风控/合规限额
  StrategyRiskLimits risk;
};

/// @brief 策略发单回调（由引擎在创建实例后注入）
using OrderSender = std::function<ErrorCode(const OrderBatch&)>;

/// @brief 策略接口类
/// @details 生命周期为 Init → Start → Stop；发单能力经 SetOrderSender 注入
class IStrategy {
 public:
  virtual ~IStrategy() = default;

  /// @brief 初始化策略
  /// @param config 策略配置
  /// @return ErrorCode::kSuccess 表示成功，其他表示失败
  virtual ErrorCode Init(const StrategyConfig& config) = 0;

  /// @brief 启动策略
  /// @return ErrorCode::kSuccess 表示成功，其他表示失败
  virtual ErrorCode Start() = 0;

  /// @brief 停止策略
  virtual void Stop() = 0;

  /// @brief 注入发单回调
  /// @param sender 发单函数
  virtual void SetOrderSender(OrderSender sender) = 0;

  /// @brief 获取策略配置
  /// @return 策略配置
  virtual StrategyConfig GetStrategyConfig() const = 0;

  /// @brief Tick数据回调
  /// @param tick 市场Tick数据
  virtual void OnTick(const qtrade::sdk::quote::MarketTick& tick) = 0;

  /// @brief Bar数据回调
  /// @param bar K线数据
  virtual void OnBar(const qtrade::sdk::quote::Bar& bar) = 0;

  /// @brief 订单更新回调
  /// @param order 订单信息
  virtual void OnOrder(const qtrade::sdk::trader::Order& order) = 0;

  /// @brief 成交更新回调
  /// @param trade 成交信息
  virtual void OnTrade(const qtrade::sdk::trader::Trade& trade) = 0;
};

}  // namespace qtrade::strategy

#endif  // QTRADE_TRADING_STRATEGY_STRATEGY_HPP_
