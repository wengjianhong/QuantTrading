/// @file      quote_types.hpp
/// @brief     行情模块枚举定义
/// @details   参考 EMT_API 行情侧类型（EMQ_EXCHANGE_TYPE / EMQ_TICKER_TYPE）。
/// @author    qtrade
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SDK_QUOTE_TYPES_HPP_
#define QTRADE_SDK_QUOTE_TYPES_HPP_

#include <cstdint>

namespace qtrade_sdk::quote {

/// @brief 行情侧交易所类型。
enum class ExchangeType : std::uint8_t {
  /// 上海证券交易所。
  kShanghai = 1,
  /// 深圳证券交易所。
  kShenzhen = 2,
  /// 北京证券交易所/股转市场。
  kBeijing = 3,
  /// 沪市港股通。
  kShanghaiHk = 4,
  /// 深市港股通。
  kShenzhenHk = 5,
  /// 未知交易所。
  kUnknown = 100,
};

/// @brief 证券类别。
enum class TickerType : std::uint8_t {
  /// 股票、基金、债券、权证、质押式回购等现货品种。
  kStock = 0,
  /// 指数。
  kIndex = 1,
  /// 期权。
  kOption = 2,
  /// 未知证券类别。
  kUnknown = 255,
};

}  // namespace qtrade_sdk::quote

#endif  // QTRADE_SDK_QUOTE_TYPES_HPP_
