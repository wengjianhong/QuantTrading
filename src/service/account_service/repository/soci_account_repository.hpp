/// @file      soci_account_repository.hpp
/// @brief     基于 SOCI 的账户仓储实现
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_SOCI_ACCOUNT_REPOSITORY_HPP_
#define QTRADE_SERVICE_SOCI_ACCOUNT_REPOSITORY_HPP_

#include "service/account_service/repository/account_repository.hpp"

#include <cpputils/database/database.hpp>

namespace qtrade::service {

class SociAccountRepository final : public IAccountRepository {
 public:
  explicit SociAccountRepository(const qtrade::common::DatabaseOptions& options);
  ~SociAccountRepository() override;

  ErrorCode EnsureSchema() override;

  ErrorCode AddAccount(const qtrade::account::v1::TradingAccount& account) override;

  ErrorCode GetAccount(const std::string& tenant_id,
                       const std::string& account_id,
                       qtrade::account::v1::TradingAccount& account) override;

  ErrorCode ListAccounts(const std::string& tenant_id,
                         std::vector<qtrade::account::v1::TradingAccount>& accounts) override;

  ErrorCode UpdateAccount(const qtrade::account::v1::TradingAccount& account) override;

  ErrorCode GetCredential(const std::string& tenant_id,
                          const std::string& engine_id,
                          const std::string& account_id,
                          qtrade::account::v1::GetCredentialResponse& response) override;

 private:
  [[nodiscard]] bool IsReady() const;

  std::unique_ptr<cpp_utils::database::IConnectionPool> pool_;
  std::unique_ptr<cpp_utils::database::IConnection> connection_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_SOCI_ACCOUNT_REPOSITORY_HPP_
