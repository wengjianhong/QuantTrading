/// @file      dml.hpp
/// @brief     单表 DML 虚接口（按记录类型泛化）
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_FRAMEWORK_DAO_DML_HPP_
#define QTRADE_FRAMEWORK_DAO_DML_HPP_

#include <qtrade/structs/result.hpp>

#include <cstdint>
#include <vector>

namespace qtrade::framework::dao {

/// @brief 单表 DML 抽象（Insert / Update / Delete / Select 等）
/// @tparam RecordT 表行对应的记录结构体类型
template <typename RecordT>
class ITableDml {
 public:
  virtual ~ITableDml() noexcept = default;

  /// @brief 插入多条记录
  /// @param records 待插入记录列表
  /// @return 受影响行数
  virtual Result<std::int64_t> Insert(const std::vector<RecordT>& records) = 0;

  /// @brief 按条件删除记录
  /// @param where_conditions 与记录结构一致的 where 条件
  /// @return 受影响行数；失败时返回 -1
  virtual Result<std::int64_t> Delete(const RecordT& where_conditions) = 0;

  /// @brief 按主键 id 列表批量删除记录
  /// @param ids 主键 id 列表
  /// @return 受影响行数；失败时返回 -1
  virtual Result<std::int64_t> BatchDelete(const std::vector<std::int64_t>& ids) = 0;

  /// @brief 按条件更新记录
  /// @param record 待写入字段
  /// @param where_conditions 更新条件
  /// @return 受影响行数；失败时返回 -1
  virtual Result<std::int64_t> Update(const RecordT& record, const RecordT& where_conditions) = 0;

  /// @brief 按条件统计记录数量
  /// @param where_conditions 查询条件
  /// @return 满足条件的行数；失败时返回 -1
  virtual Result<std::int64_t> Count(const RecordT& where_conditions) = 0;

  /// @brief 按条件查询记录列表
  /// @param where_conditions 查询条件
  /// @return 查询结果（含错误码与数据）
  virtual Result<std::vector<RecordT>> Select(const RecordT& where_conditions) = 0;

  /// @brief 清空表全部记录
  /// @return 受影响行数；失败时返回 -1
  virtual Result<std::int64_t> Truncate() = 0;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_FRAMEWORK_DAO_DML_HPP_
