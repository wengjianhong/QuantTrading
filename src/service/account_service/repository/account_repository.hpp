/// @file      account_repository.hpp
/// @brief     交易账户持久化抽象
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_REPOSITORY_HPP_
#define QTRADE_SERVICE_ACCOUNT_REPOSITORY_HPP_

#include "common/database/database_options.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/account/v1/account.pb.h>

#include <memory>
#include <string>
#include <vector>

namespace qtrade::service {

/// @brief 清除 TradingAccount 中的 password 字段（查询响应用）
void StripAccountPassword(qtrade::account::v1::TradingAccount& account);

/// @brief 交易账户读写仓储接口
class IAccountRepository {
 public:
  virtual ~IAccountRepository() = default;

  virtual ErrorCode EnsureSchema() = 0;

  virtual ErrorCode AddAccount(const qtrade::account::v1::TradingAccount& account) = 0;

  virtual ErrorCode GetAccount(const std::string& tenant_id,
                               const std::string& account_id,
                               qtrade::account::v1::TradingAccount& account) = 0;

  virtual ErrorCode ListAccounts(const std::string& tenant_id,
                                 std::vector<qtrade::account::v1::TradingAccount>& accounts) = 0;

  virtual ErrorCode UpdateAccount(const qtrade::account::v1::TradingAccount& account) = 0;

  virtual ErrorCode GetCredential(const std::string& tenant_id,
                                  const std::string& engine_id,
                                  const std::string& account_id,
                                  qtrade::account::v1::GetCredentialResponse& response) = 0;
};

[[nodiscard]] std::shared_ptr<IAccountRepository> CreateAccountRepository(
    const qtrade::common::DatabaseOptions& options);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_REPOSITORY_HPP_
