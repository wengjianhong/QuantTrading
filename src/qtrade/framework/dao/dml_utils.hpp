/// @file      dml_utils.hpp
/// @brief     DML 公共工具（连接、KeyValues、CRUD 辅助）
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DAO_DML_UTILS_HPP_
#define QTRADE_COMMON_DAO_DML_UTILS_HPP_

#include <qtrade/structs/result.hpp>

#include <cpputils/database/connection.hpp>
#include <cpputils/database/result_row.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace qtrade::framework::dao {

/// 列名与字符串值的键值对
using KeyValue = std::pair<std::string, std::string>;
/// 用于 INSERT/UPDATE/WHERE 的列值列表
using KeyValues = std::vector<KeyValue>;

/// @brief 将 optional 文本字段追加到 KeyValues（无值时跳过）
/// @param out 目标 KeyValues
/// @param column 列名
/// @param value 字段值
void AddTextValue(KeyValues& out, const char* column, const std::optional<std::string>& value);

/// @brief 将 optional int64 字段追加到 KeyValues（无值时跳过）
/// @param out 目标 KeyValues
/// @param column 列名
/// @param value 字段值
void AddInt64Value(KeyValues& out, const char* column, const std::optional<std::int64_t>& value);

/// @brief 将 optional uint64 字段追加到 KeyValues（无值时跳过）
/// @param out 目标 KeyValues
/// @param column 列名
/// @param value 字段值
void AddUInt64Value(KeyValues& out, const char* column, const std::optional<std::uint64_t>& value);

/// @brief 从结果行读取文本列到 optional 字段
/// @param row 数据库结果行
/// @param column 列名
/// @param field 输出字段（有值时写入）
void AssignTextField(const cpputils::database::IResultRow& row, const char* column, std::optional<std::string>& field);

/// @brief 从结果行读取 int64 列到 optional 字段
/// @param row 数据库结果行
/// @param column 列名
/// @param field 输出字段（有值时写入）
void AssignInt64Field(const cpputils::database::IResultRow& row,
                      const char* column,
                      std::optional<std::int64_t>& field);

/// @brief 从结果行读取 uint64 列到 optional 字段
/// @param row 数据库结果行
/// @param column 列名
/// @param field 输出字段（有值时写入）
void AssignUInt64Field(const cpputils::database::IResultRow& row,
                       const char* column,
                       std::optional<std::uint64_t>& field);

/// @brief 由 KeyValues 构建 WHERE 子句（含 WHERE 关键字）
/// @param where_values 条件列值；空列表返回空字符串
/// @return SQL WHERE 片段，如 " WHERE col = 'v'"
[[nodiscard]] std::string BuildWhereSql(const KeyValues& where_values);

/// @brief 向指定表插入一行
/// @param connection 请求或事务独占的数据库连接
/// @param table 表名
/// @param values 待插入列值
/// @return 成功时 data 为 1（单行插入）
[[nodiscard]] Result<std::int64_t> InsertRow(cpputils::database::IConnection& connection,
                                             const std::string& table,
                                             const KeyValues& values);

/// @brief 按条件删除行
/// @param connection 请求或事务独占的数据库连接
/// @param table 表名
/// @param where_values 删除条件（不可为空）
/// @return 成功时 data 为受影响行数
[[nodiscard]] Result<std::int64_t> DeleteRows(cpputils::database::IConnection& connection,
                                              const std::string& table,
                                              const KeyValues& where_values);

/// @brief 按条件更新行
/// @param connection 请求或事务独占的数据库连接
/// @param table 表名
/// @param values SET 列值（不可为空）
/// @param where_values WHERE 条件（不可为空）
/// @return 成功时 data 为受影响行数
[[nodiscard]] Result<std::int64_t> UpdateRows(cpputils::database::IConnection& connection,
                                              const std::string& table,
                                              const KeyValues& values,
                                              const KeyValues& where_values);

/// @brief 按条件统计行数
/// @param connection 请求或事务独占的数据库连接
/// @param table 表名
/// @param where_values 查询条件；空列表表示全表计数
/// @return 成功时 data 为行数
[[nodiscard]] Result<std::int64_t> CountRows(cpputils::database::IConnection& connection,
                                             const std::string& table,
                                             const KeyValues& where_values);

/// @brief 按条件查询行
/// @param connection 请求或事务独占的数据库连接
/// @param table 表名
/// @param where_values 查询条件；空列表表示全表查询
/// @return 成功时 data 为结果集
[[nodiscard]] Result<std::unique_ptr<cpputils::database::IResultSet>> SelectRows(
  cpputils::database::IConnection& connection, const std::string& table, const KeyValues& where_values);

/// @brief 清空表全部记录
/// @param connection 请求或事务独占的数据库连接
/// @param table 表名
/// @return 成功时 data 为受影响行数
[[nodiscard]] Result<std::int64_t> TruncateRows(cpputils::database::IConnection& connection, const std::string& table);

}  // namespace qtrade::framework::dao

#endif  // QTRADE_COMMON_DAO_DML_UTILS_HPP_
