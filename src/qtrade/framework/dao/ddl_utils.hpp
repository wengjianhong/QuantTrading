/// @file      ddl_utils.hpp
/// @brief     DDL 公共工具（建表与索引执行）
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DAO_DDL_UTILS_HPP_
#define QTRADE_COMMON_DAO_DDL_UTILS_HPP_

#include <qtrade/dao/ddl.hpp>
#include <qtrade/structs/result.hpp>

#include <cpputils/database/connection.hpp>

namespace qtrade::framework::dao {

/// @brief 确保单表 schema 已创建（建表 + 索引）
/// @param connection 数据库连接；不可为 nullptr 且须已连接
/// @param schema 表 schema 元数据
/// @return 成功返回 kSuccess；连接无效或 DDL 执行失败返回 kSystemError
[[nodiscard]] ErrorCode EnsureTableSchema(cpputils::database::IConnection* connection, const ITableDdl& schema);

}  // namespace qtrade::framework::dao

#endif  // QTRADE_COMMON_DAO_DDL_UTILS_HPP_
