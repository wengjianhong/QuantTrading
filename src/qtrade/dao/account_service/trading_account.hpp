/// @file      trading_account.hpp
/// @brief     trading_account 表 DAO 声明
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_TRADING_ACCOUNT_HPP_
#define QTRADE_DAO_TRADING_ACCOUNT_HPP_
#include "qtrade/framework/dao/dml_utils.hpp"

#include <qtrade/dao/ddl.hpp>
#include <qtrade/dao/dml.hpp>

#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief trading_account 表行记录
/// @details 表主键说明：(tenant_id, account_id)
///
struct TradingAccountRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 账户 ID
  std::optional<std::string> account_id;
  /// 券商 ID
  std::optional<std::string> broker_id;
  /// 连接串
  std::optional<std::string> connection_string;
  /// 账户状态（如 active / disabled）
  std::optional<std::string> status;
};

/// @brief trading_account 表 DAO
class TradingAccount final : public ITableDml<TradingAccountRecord>, public ITableDdl {
 public:
  TradingAccount() = default;
  TradingAccount(TradingAccount&&) = delete;
  TradingAccount(const TradingAccount&) = delete;
  TradingAccount& operator=(TradingAccount&&) = delete;
  TradingAccount& operator=(const TradingAccount&) = delete;
  ~TradingAccount() noexcept override = default;

  /// ========================= ITableDdl 接口实现 =========================
  /// @brief 获取逻辑数据库名
  /// @return 固定为 "account"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "trading_account"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;

  /// ========================= ITableDml 接口实现 =========================
  /// @brief 插入交易账户记录
  /// @param records 待插入记录
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Insert(cpputils::database::IConnection& connection,
                              const std::vector<TradingAccountRecord>& records) override;

  /// @brief 按条件删除账户
  /// @param where_conditions 删除条件
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Delete(cpputils::database::IConnection& connection,
                              const TradingAccountRecord& where_conditions) override;

  /// @brief 按主键 id 批量删除（本表为复合主键，不支持）
  /// @param ids 主键 id 列表（未使用）
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> BatchDelete(cpputils::database::IConnection& connection,
                                   const std::vector<std::int64_t>& ids) override;

  /// @brief 按条件更新账户
  /// @param record 待写入字段
  /// @param where_conditions 更新条件
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Update(cpputils::database::IConnection& connection,
                              const TradingAccountRecord& record,
                              const TradingAccountRecord& where_conditions) override;

  /// @brief 按条件统计账户数量
  /// @param where_conditions 查询条件
  /// @return 成功：result.data 为行数；失败：result.error_code 为错误码
  Result<std::int64_t> Count(cpputils::database::IConnection& connection,
                             const TradingAccountRecord& where_conditions) override;

  /// @brief 按条件查询账户列表
  /// @param where_conditions 查询条件
  /// @return 查询结果；成功：result.data 为查询结果；失败：result.error_code 为错误码
  Result<std::vector<TradingAccountRecord>> Select(cpputils::database::IConnection& connection,
                                                   const TradingAccountRecord& where_conditions) override;

  /// @brief 清空表全部记录
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Truncate(cpputils::database::IConnection& connection) override;
};

/// @brief 将 TradingAccountRecord 转为 KeyValues
/// @param record 源记录
/// @return 用于 SQL 的列值列表
KeyValues BuildTradingAccountValues(const TradingAccountRecord& record);

/// @brief 从数据库结果行构建 TradingAccountRecord
/// @tparam RowT 行类型
/// @param row 结果行
/// @return 解析后的记录
template <typename RowT>
TradingAccountRecord BuildTradingAccountRecord(const RowT& row) {
  TradingAccountRecord record;
  AssignTextField(row, "tenant_id", record.tenant_id);
  AssignTextField(row, "account_id", record.account_id);
  AssignTextField(row, "broker_id", record.broker_id);
  AssignTextField(row, "connection_string", record.connection_string);
  AssignTextField(row, "status", record.status);
  return record;
}

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_TRADING_ACCOUNT_HPP_
