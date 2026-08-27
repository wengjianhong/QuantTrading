/// @file      trader_types.hpp
/// @brief     交易侧枚举合法性校验
/// @author    wengjianhong
/// @date      2026-08-27
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_VALIDATE_TRADER_TYPES_HPP_
#define QTRADE_COMMON_VALIDATE_TRADER_TYPES_HPP_

#include <qtrade/sdk/trader/trader_types.hpp>

namespace qtrade::common::validate {

/// @brief 是否为可发单的交易市场（排除 kInit / kUnknown 及未定义取值）
/// @param market 交易市场
/// @return 合法时返回 true
[[nodiscard]] inline bool IsValidMarket(qtrade::sdk::trader::MarketType market) {
  switch (market) {
    case qtrade::sdk::trader::MarketType::kShA:
    case qtrade::sdk::trader::MarketType::kSzA:
    case qtrade::sdk::trader::MarketType::kShHkConnect:
    case qtrade::sdk::trader::MarketType::kSzHkConnect:
    case qtrade::sdk::trader::MarketType::kBjA:
      return true;
    default:
      return false;
  }
}

}  // namespace qtrade::common::validate

#endif  // QTRADE_COMMON_VALIDATE_TRADER_TYPES_HPP_
