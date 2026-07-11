/// @file      sql_utils.hpp
/// @brief     DAO SQL 工具函数
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DAO_SQL_UTILS_HPP_
#define QTRADE_COMMON_DAO_SQL_UTILS_HPP_

#include <sstream>
#include <string>

namespace qtrade::framework::dao {

/// @brief 转义 SQL 字符串字面量中的单引号
/// @param value 原始字符串
/// @return 可用于 SQL 字符串字面量中的单引号转义结果
[[nodiscard]] std::string EscapeSqlLiteral(const std::string& value);

/// @brief 向 SQL 流追加字符串等值条件
/// @param sql 目标 SQL 流
/// @param column 列名
/// @param value 条件值；空字符串时不追加
/// @param has_where 是否已有 WHERE；首次追加时写入 WHERE，否则写入 AND
void AppendStringEq(std::ostringstream& sql, const char* column, const std::string& value, bool& has_where);

}  // namespace qtrade::framework::dao

#endif  // QTRADE_COMMON_DAO_SQL_UTILS_HPP_
