/// @file      soci_account_repository.hpp
/// @brief     账户仓储（DAO 编排层）
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_SOCI_ACCOUNT_REPOSITORY_HPP_
#define QTRADE_SERVICE_SOCI_ACCOUNT_REPOSITORY_HPP_

#include "qtrade_framework/common/database/db_connection.hpp"
#include "qtrade/service/account_service/repository/account_repository.hpp"

namespace qtrade::service {

/// @brief 基于 TradingAccount / AccountCredential DAO 的账户仓储实现
class SociAccountRepository final : public IAccountRepository {
 public:
  /// @brief 构造并尝试建立数据库连接
  /// @param options 数据库连接选项
  explicit SociAccountRepository(const qtrade::common::DatabaseOptions& options);

  /// @brief 析构并释放连接
  ~SociAccountRepository() override;

  /// @brief 确保 trading_account 与 account_credential 表 schema 存在
  ErrorCode EnsureSchema() override;

  /// @brief 新增交易账户及加密凭证
  /// @param account proto 账户（含明文密码）
  ErrorCode AddAccount(const qtrade::account::v1::TradingAccount& account) override;

  /// @brief 按 tenant_id + account_id 查询账户
  ErrorCode GetAccount(const std::string& tenant_id,
                       const std::string& account_id,
                       qtrade::account::v1::TradingAccount& account) override;

  /// @brief 列出租户下全部账户
  /// @param tenant_id 租户 ID；空表示全表
  ErrorCode ListAccounts(const std::string& tenant_id,
                         std::vector<qtrade::account::v1::TradingAccount>& accounts) override;

  /// @brief 更新账户信息；password 非空时同步更新凭证
  ErrorCode UpdateAccount(const qtrade::account::v1::TradingAccount& account) override;

  /// @brief 获取含解密密码的账户凭证（供引擎拉取）
  /// @param tenant_id 租户 ID
  /// @param engine_id 引擎 ID（审计日志用）
  /// @param account_id 账户 ID
  /// @param response 输出响应
  ErrorCode GetCredential(const std::string& tenant_id,
                          const std::string& engine_id,
                          const std::string& account_id,
                          qtrade::account::v1::GetCredentialResponse& response) override;

 private:
  /// @brief 判断数据库连接是否就绪
  [[nodiscard]] bool IsReady() const;

  qtrade::framework::dao::DbConnectionHolder connection_;  ///< 数据库连接持有者
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_SOCI_ACCOUNT_REPOSITORY_HPP_
