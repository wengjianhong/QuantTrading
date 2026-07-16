/// @file      account_credential.hpp
/// @brief     account_credential 表 DAO 声明
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_ACCOUNT_CREDENTIAL_HPP_
#define QTRADE_DAO_ACCOUNT_CREDENTIAL_HPP_

#include "qtrade/framework/dao/dml_utils.hpp"

#include <qtrade/dao/ddl.hpp>
#include <qtrade/dao/dml.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief 凭证类型
enum class CredentialType {
  kDefault = 0,
  kPassword = 1,
  kAuthCode = 2,
};

/// @brief account_credential 表行记录
/// @details 表主键说明：(tenant_id, account_id, key_id, credential_type)
///
struct AccountCredentialRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 账户 ID
  std::optional<std::string> account_id;
  /// 加密密钥标识
  std::optional<std::string> key_id;
  /// 凭证类型（同账户可多行）
  std::optional<CredentialType> credential_type;
  /// 凭证密文
  std::optional<std::string> ciphertext;
};

/// @brief account_credential 表 DAO（单例）
class AccountCredential final : public ITableDml<AccountCredentialRecord>, public ITableDdl {
 public:
  AccountCredential() = default;
  AccountCredential(AccountCredential&&) = delete;
  AccountCredential(const AccountCredential&) = delete;
  AccountCredential& operator=(AccountCredential&&) = delete;
  AccountCredential& operator=(const AccountCredential&) = delete;
  ~AccountCredential() noexcept override = default;

  /// @brief 获取单例实例
  static AccountCredential& Instance();

  /// @brief 设置测试用 Mock 实例
  /// @param mock_instance Mock 对象指针
  static void SetMockInstance(AccountCredential* mock_instance);

  /// @brief 清除测试用 Mock 实例
  static void ClearMockInstance();

  /// ========================= ITableDdl 接口实现 =========================
  /// @brief 获取逻辑表名
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  const std::vector<std::string>& GetIndexSqls() const override;

  /// ========================= ITableDml 接口实现 =========================
  /// @brief 插入凭证记录
  /// @param records 待插入记录
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Insert(const std::vector<AccountCredentialRecord>& records) override;

  /// @brief 按条件删除凭证
  /// @param where_conditions 删除条件
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Delete(const AccountCredentialRecord& where_conditions) override;

  /// @brief 按主键 id 批量删除（本表为复合主键，不支持）
  /// @param ids 主键 id 列表（未使用）
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> BatchDelete(const std::vector<std::int64_t>& ids) override;

  /// @brief 按条件更新凭证
  /// @param record 待写入字段
  /// @param where_conditions 更新条件
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Update(const AccountCredentialRecord& record,
                              const AccountCredentialRecord& where_conditions) override;

  /// @brief 按条件统计凭证数量
  /// @param where_conditions 查询条件
  /// @return 成功：result.data 为行数；失败：result.error_code 为错误码
  Result<std::int64_t> Count(const AccountCredentialRecord& where_conditions) override;

  /// @brief 按条件查询凭证列表
  /// @param where_conditions 查询条件
  /// @return 查询结果；成功：result.data 为查询结果；失败：result.error_code 为错误码
  Result<std::vector<AccountCredentialRecord>> Select(const AccountCredentialRecord& where_conditions) override;

  /// @brief 清空表全部记录
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Truncate() override;
};

/// @brief 将 AccountCredentialRecord 转为 KeyValues
/// @param record 源记录
/// @return 用于 SQL 的列值列表
KeyValues BuildAccountCredentialValues(const AccountCredentialRecord& record);

/// @brief 从数据库结果行构建 AccountCredentialRecord
/// @tparam RowT 行类型
/// @param row 结果行
/// @return 解析后的记录
template <typename RowT>
AccountCredentialRecord BuildAccountCredentialRecord(const RowT& row) {
  AccountCredentialRecord record;
  AssignTextField(row, "tenant_id", record.tenant_id);
  AssignTextField(row, "account_id", record.account_id);
  std::optional<std::int64_t> credential_type;
  AssignInt64Field(row, "credential_type", credential_type);
  if (credential_type.has_value()) {
    record.credential_type = static_cast<CredentialType>(credential_type.value());
  }
  AssignTextField(row, "key_id", record.key_id);
  AssignTextField(row, "ciphertext", record.ciphertext);
  return record;
}

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_ACCOUNT_CREDENTIAL_HPP_
