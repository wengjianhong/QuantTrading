/// @file      strategy_config_utils.hpp
/// @brief     config.v1.StrategyConfig 解析工具
/// @author    wengjianhong
/// @date      2026-07-31
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_PROTO_STRATEGY_CONFIG_UTILS_HPP_
#define QTRADE_COMMON_PROTO_STRATEGY_CONFIG_UTILS_HPP_

#include <qtrade/proto/config/v1/config.pb.h>
#include <qtrade/strategy/strategy.hpp>

namespace qtrade::common::proto {
using StrategyConfigProto = qtrade::config::v1::StrategyConfig;

/// @brief 将控制面 proto 策略配置转为策略运行时配置
/// @param config 策略配置的 protobuf 对象
/// @return 策略配置的结构体
[[nodiscard]] qtrade::strategy::StrategyConfig ParseStrategyConfigProto(const StrategyConfigProto& config);

}  // namespace qtrade::common::proto

#endif  // QTRADE_COMMON_PROTO_STRATEGY_CONFIG_UTILS_HPP_
