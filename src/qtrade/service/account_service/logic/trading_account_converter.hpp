/// @file      trading_account_converter.hpp
/// @brief     TradingAccount proto 与 DAO 记录互转
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_TRADING_ACCOUNT_CONVERTER_HPP_
#define QTRADE_SERVICE_ACCOUNT_TRADING_ACCOUNT_CONVERTER_HPP_

#include "qtrade/dao/account_service/trading_account.hpp"

#include <qtrade/proto/account/v1/account.pb.h>

namespace qtrade::service {

[[nodiscard]] qtrade::framework::dao::TradingAccountRecord ToTradingAccountRecord(
  const qtrade::account::v1::TradingAccount& account);

void ToTradingAccountProto(const qtrade::framework::dao::TradingAccountRecord& record,
                           qtrade::account::v1::TradingAccount& account);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_TRADING_ACCOUNT_CONVERTER_HPP_
