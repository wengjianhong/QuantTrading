/// @file      account_repository.cpp
/// @brief     账户仓储工厂
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "service/account_service/repository/account_repository.hpp"

#include "service/account_service/repository/soci_account_repository.hpp"

namespace qtrade::service {

void StripAccountPassword(qtrade::account::v1::TradingAccount& account) {
  account.set_password("");
}

std::shared_ptr<IAccountRepository> CreateAccountRepository(const qtrade::common::DatabaseOptions& options) {
  if (!options.enabled) {
    return nullptr;
  }
  return std::make_shared<SociAccountRepository>(options);
}

}  // namespace qtrade::service
