/// @file      dml.hpp
/// @brief     单表 DML 虚接口（按记录类型泛化）
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_FRAMEWORK_DAO_DML_HPP_
#define QTRADE_FRAMEWORK_DAO_DML_HPP_

#include <qtrade/structs/result.hpp>

#include <cpputils/database/connection.hpp>

#include <cstdint>
#include <vector>

namespace qtrade::framework::dao {

/// @brief 单表 DML 抽象接口
/// @tparam RecordT 表行记录类型；字段通常为 std::optional，便于部分条件查询
template <typename RecordT>
class ITableDml {
 public:
  virtual ~ITableDml() noexcept = default;

  /// @brief 插入多条记录
  /// @param records 待插入记录列表
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  virtual Result<std::int64_t> Insert(cpputils::database::IConnection& connection,
                                      const std::vector<RecordT>& records) = 0;

  /// @brief 按条件删除记录
  /// @param where_conditions 查询条件（仅填需匹配的 optional 字段）
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  virtual Result<std::int64_t> Delete(cpputils::database::IConnection& connection, const RecordT& where_conditions) = 0;

  /// @brief 按主键 id 列表批量删除
  /// @param ids 主键 id 列表
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  virtual Result<std::int64_t> BatchDelete(cpputils::database::IConnection& connection,
                                           const std::vector<std::int64_t>& ids) = 0;

  /// @brief 按条件更新记录
  /// @param record 待写入字段
  /// @param where_conditions 更新条件
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  virtual Result<std::int64_t> Update(cpputils::database::IConnection& connection,
                                      const RecordT& record,
                                      const RecordT& where_conditions) = 0;

  /// @brief 按条件统计记录数
  /// @param where_conditions 查询条件；全空 optional 表示全表
  /// @return 成功：result.data 为行数；失败：result.error_code 为错误码
  virtual Result<std::int64_t> Count(cpputils::database::IConnection& connection, const RecordT& where_conditions) = 0;

  /// @brief 按条件查询记录列表
  /// @param where_conditions 查询条件；全空 optional 表示全表
  /// @return 查询结果；成功：result.data 为查询结果；失败：result.error_code 为错误码
  virtual Result<std::vector<RecordT>> Select(cpputils::database::IConnection& connection,
                                              const RecordT& where_conditions) = 0;

  /// @brief 清空表全部记录
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  virtual Result<std::int64_t> Truncate(cpputils::database::IConnection& connection) = 0;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_FRAMEWORK_DAO_DML_HPP_
