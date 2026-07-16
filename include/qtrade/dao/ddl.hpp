/// @file      ddl.hpp
/// @brief     单表 DDL 元数据抽象（表名、建表 SQL、索引 SQL）
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_FRAMEWORK_DAO_DDL_HPP_
#define QTRADE_FRAMEWORK_DAO_DDL_HPP_

#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief 单表 DDL 抽象
/// @details 由各表 DAO 实现，暴露表名与 DDL 脚本，供 ddl_utils 统一调用
class ITableDdl {
 public:
  virtual ~ITableDdl() noexcept = default;

  /// @brief 获取逻辑数据库名
  /// @return 服务内连接路由键；通常与 JSON database_name 一致
  /// @details 用于 SetConnection / GetConnection 选择连接，不代表跨环境物理库名映射
  virtual const std::string& DatabaseName() const = 0;

  /// @brief 获取逻辑表名
  /// @return 与数据库中实际表名一致
  virtual const std::string& TableName() const = 0;

  /// @brief 建表 SQL 语句列表
  virtual const std::vector<std::string>& GetCreateTableSqls() const = 0;

  /// @brief 索引 SQL 语句列表
  virtual const std::vector<std::string>& GetIndexSqls() const = 0;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_FRAMEWORK_DAO_DDL_HPP_
