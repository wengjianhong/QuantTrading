/// @file      engine_config.hpp
/// @brief     engine_config 表 DAO 声明
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_DAO_ENGINE_CONFIG_HPP_
#define QTRADE_DAO_ENGINE_CONFIG_HPP_
#include "qtrade/framework/dao/dml_utils.hpp"

#include <qtrade/dao/ddl.hpp>
#include <qtrade/dao/dml.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qtrade::framework::dao {

/// @brief engine_config 表行记录
/// @details 表主键说明：(tenant_id, engine_id)
///
struct EngineConfigRecord {
  /// 租户 ID
  std::optional<std::string> tenant_id;
  /// 引擎 ID
  std::optional<std::string> engine_id;
  /// 配置版本号
  std::optional<std::uint64_t> version;
  /// JSON 配置载荷
  std::optional<std::string> payload;
};

/// @brief engine_config 表 DAO
/// @details 同时实现 ITableDml 与 ITableDdl；DML 调用显式传入请求级连接
class EngineConfig final : public ITableDml<EngineConfigRecord>, public ITableDdl {
 public:
  EngineConfig() = default;
  EngineConfig(EngineConfig&&) = delete;
  EngineConfig(const EngineConfig&) = delete;
  EngineConfig& operator=(EngineConfig&&) = delete;
  EngineConfig& operator=(const EngineConfig&) = delete;
  ~EngineConfig() noexcept override = default;

  /// ========================= ITableDdl 接口实现 =========================
  /// @brief 获取逻辑数据库名
  /// @return 固定为 "config"
  const std::string& DatabaseName() const override;

  /// @brief 获取逻辑表名
  /// @return 固定为 "engine_config"
  const std::string& TableName() const override;

  /// @brief 获取建表 SQL 列表
  /// @return 含 CREATE TABLE 语句的列表
  const std::vector<std::string>& GetCreateTableSqls() const override;

  /// @brief 获取索引 SQL 列表
  /// @return 索引语句列表；无索引时为空
  const std::vector<std::string>& GetIndexSqls() const override;

  /// ========================= ITableDml 接口实现 =========================
  /// @brief 插入配置记录
  /// @param records 待插入记录
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Insert(cpputils::database::IConnection& connection,
                              const std::vector<EngineConfigRecord>& records) override;

  /// @brief 按条件删除配置
  /// @param where_conditions 删除条件（不可全空）
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Delete(cpputils::database::IConnection& connection,
                              const EngineConfigRecord& where_conditions) override;

  /// @brief 按主键 id 批量删除（本表为复合主键，不支持）
  /// @param ids 主键 id 列表（未使用）
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> BatchDelete(cpputils::database::IConnection& connection,
                                   const std::vector<std::int64_t>& ids) override;

  /// @brief 按条件更新配置
  /// @param record 待写入字段
  /// @param where_conditions 更新条件
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Update(cpputils::database::IConnection& connection,
                              const EngineConfigRecord& record,
                              const EngineConfigRecord& where_conditions) override;

  /// @brief 按条件统计配置数量
  /// @param where_conditions 查询条件
  /// @return 成功：result.data 为行数；失败：result.error_code 为错误码
  Result<std::int64_t> Count(cpputils::database::IConnection& connection,
                             const EngineConfigRecord& where_conditions) override;

  /// @brief 按条件查询配置列表
  /// @param where_conditions 查询条件
  /// @return 查询结果；成功：result.data 为查询结果；失败：result.error_code 为错误码
  Result<std::vector<EngineConfigRecord>> Select(cpputils::database::IConnection& connection,
                                                 const EngineConfigRecord& where_conditions) override;

  /// @brief 清空表全部记录
  /// @return 成功：result.data 为受影响行数；失败：result.error_code 为错误码
  Result<std::int64_t> Truncate(cpputils::database::IConnection& connection) override;
};

/// @brief 将 EngineConfigRecord 转为 KeyValues
/// @param record 源记录
/// @return 用于 SQL 的列值列表
KeyValues BuildEngineConfigValues(const EngineConfigRecord& record);

/// @brief 从数据库结果行构建 EngineConfigRecord
/// @tparam RowT 行类型（需支持 get_value）
/// @param row 结果行
/// @return 解析后的记录
template <typename RowT>
EngineConfigRecord BuildEngineConfigRecord(const RowT& row) {
  EngineConfigRecord record;
  AssignTextField(row, "tenant_id", record.tenant_id);
  AssignTextField(row, "engine_id", record.engine_id);
  AssignUInt64Field(row, "version", record.version);
  AssignTextField(row, "payload", record.payload);
  return record;
}

}  // namespace qtrade::framework::dao

#endif  // QTRADE_DAO_ENGINE_CONFIG_HPP_
