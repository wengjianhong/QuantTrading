/// @file      account_repository.hpp
/// @brief     交易账户持久化抽象
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_REPOSITORY_HPP_
#define QTRADE_SERVICE_ACCOUNT_REPOSITORY_HPP_

#include "common/database/database_options.hpp"

#include <qtrade/proto/account/v1/account.pb.h>
#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>
#include <vector>

namespace qtrade::service {

/// @brief 交易账户读写仓储接口
class IAccountRepository {
 public:
  virtual ~IAccountRepository() = default;

  virtual ErrorCode EnsureSchema() = 0;

  virtual ErrorCode RegisterAccount(const qtrade::account::v1::TradingAccount& account,
                                    const std::string& password) = 0;

  virtual ErrorCode RotateCredential(const std::string& account_id, const std::string& password) = 0;

  virtual ErrorCode BindAccountToEngine(const std::string& account_id, const std::string& engine_id) = 0;

  virtual ErrorCode ListAccounts(const std::string& tenant_id,
                                 std::vector<qtrade::account::v1::TradingAccount>& accounts) = 0;

  virtual ErrorCode ResolveCredential(const std::string& engine_id,
                                      const std::string& account_id,
                                      qtrade::account::v1::ResolveCredentialResponse& response) = 0;
};

[[nodiscard]] std::shared_ptr<IAccountRepository> CreateAccountRepository(
    const qtrade::common::DatabaseOptions& options);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_ACCOUNT_REPOSITORY_HPP_
