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
#include <optional>
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
  /// 单笔下单量
  std::int64_t order_volume = 0;
  /// 最大持仓（绝对值）
  std::int64_t max_position_volume = 0;
  /// 发单冷却（毫秒）
  std::int32_t order_cooldown_ms = 0;

  // ========== 可选参数 ==========
  /// 策略计算窗口(K线/Tick样本数量)
  std::optional<std::int32_t> window_size;
  /// 下单信号阈值（信号强度大于此值时才发单）
  std::optional<double> order_threshold;
  /// 止损比例（持仓亏损超过此比例时止损）
  std::optional<double> stop_loss_percent;
  /// 止盈比例（持仓盈利超过此比例时止盈）
  std::optional<double> take_profit_percent;
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
