/// @file      strategy.hpp
/// @brief     策略接口定义
/// @details   定义策略接口 IStrategy 以及相关数据结构
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_STRATEGY_STRATEGY_HPP_
#define QTRADE_TRADING_STRATEGY_STRATEGY_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade_sdk/quote/quote_struct.hpp>
#include <qtrade_sdk/trader/trader_struct.hpp>

#include <string>

namespace qtrade::strategy {

/// @brief 策略配置结构体
/// @details 包含策略名称和参数配置
struct StrategyConfig {
  std::string name;            /// 策略名称
  std::string parameter_blob;  /// 参数配置（JSON格式或其他序列化格式）
};

/// @brief 交易信号结构体
/// @details 策略产生的交易信号
struct Signal {
  std::string instrument;  /// 合约代码
  int direction{};         /// 方向：-1=做空，0=平仓，1=做多
  double strength{};       /// 信号强度（0.0~1.0）
};

/// @brief 策略接口类
/// @details 所有策略必须实现此接口
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

  /// @brief 暂停策略
  virtual void Pause() = 0;

  /// @brief 恢复策略
  virtual void Resume() = 0;

  /// @brief 停止策略
  virtual void Stop() = 0;

  /// @brief Tick数据回调
  /// @param tick 市场Tick数据
  virtual void OnTick(const qtrade_sdk::quote::MarketTick& tick) = 0;

  /// @brief Bar数据回调
  /// @param bar K线数据
  virtual void OnBar(const qtrade_sdk::quote::Bar& bar) = 0;

  /// @brief 订单更新回调
  /// @param order 订单信息
  virtual void OnOrder(const qtrade_sdk::trader::Order& order) = 0;

  /// @brief 成交更新回调
  /// @param trade 成交信息
  virtual void OnTrade(const qtrade_sdk::trader::Trade& trade) = 0;

  /// @brief 发送交易信号
  /// @param signal 交易信号
  /// @return ErrorCode::kSuccess 表示成功，其他表示失败
  virtual ErrorCode SendSignal(const Signal& signal) = 0;

  /// @brief 获取策略参数
  /// @param key 参数名
  /// @return 参数值
  virtual std::string GetParameter(const std::string& key) const = 0;

  /// @brief 设置策略参数
  /// @param key 参数名
  /// @param value 参数值
  /// @return ErrorCode::kSuccess 表示成功，其他表示失败
  virtual ErrorCode SetParameter(const std::string& key, const std::string& value) = 0;
};

}  // namespace qtrade::strategy

#endif  // QTRADE_TRADING_STRATEGY_STRATEGY_HPP_
