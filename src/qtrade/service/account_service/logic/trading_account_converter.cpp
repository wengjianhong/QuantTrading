/// @file      trading_account_converter.cpp
/// @brief     TradingAccount proto 与 DAO 记录互转实现
/// @author    wengjianhong
/// @date      2026-07-13
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/service/account_service/logic/trading_account_converter.hpp"

namespace qtrade::service {

qtrade::framework::dao::TradingAccountRecord ToTradingAccountRecord(
  const qtrade::account::v1::TradingAccount& account) {
  qtrade::framework::dao::TradingAccountRecord record;
  record.tenant_id = account.tenant_id();
  record.account_id = account.account_id();
  record.broker_id = account.broker_id();
  record.connection_string = account.connection_string();
  record.status = account.status().empty() ? "active" : account.status();
  return record;
}

void ToTradingAccountProto(const qtrade::framework::dao::TradingAccountRecord& record,
                           qtrade::account::v1::TradingAccount& account) {
  account.set_tenant_id(record.tenant_id.value_or(""));
  account.set_account_id(record.account_id.value_or(""));
  account.set_broker_id(record.broker_id.value_or(""));
  account.set_connection_string(record.connection_string.value_or(""));
  account.set_status(record.status.value_or(""));
}

}  // namespace qtrade::service
