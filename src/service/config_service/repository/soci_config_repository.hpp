/// @file      soci_config_repository.hpp
/// @brief     配置仓储（DAO 编排层）
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_SOCI_CONFIG_REPOSITORY_HPP_
#define QTRADE_SERVICE_SOCI_CONFIG_REPOSITORY_HPP_

#include "service/config_service/repository/config_repository.hpp"

#include "common/database/db_connection.hpp"

namespace qtrade::service {

/// @brief 基于 EngineConfig DAO 的配置仓储实现
/// @details 负责连接持有、schema 初始化及 proto 与 DAO 记录之间的编排
class SociConfigRepository final : public IConfigRepository {
 public:
  /// @brief 构造并尝试建立数据库连接
  /// @param options 数据库连接选项
  explicit SociConfigRepository(const qtrade::common::DatabaseOptions& options);

  /// @brief 析构并释放连接
  ~SociConfigRepository() override;

  /// @brief 确保 engine_config 表 schema 存在
  /// @return 成功返回 kSuccess
  ErrorCode EnsureSchema() override;

  /// @brief 加载指定 scope 的引擎配置
  /// @param scope 租户与引擎标识
  /// @param config 输出 proto 配置
  /// @param version 输出配置版本号
  /// @return 成功 kSuccess；不存在 kNotFound
  ErrorCode Load(const ConfigScope& scope, qtrade::config::v1::EngineConfig& config, std::uint64_t& version) override;

  /// @brief 保存引擎配置（存在则更新，否则插入）
  /// @param scope 租户与引擎标识
  /// @param config proto 配置
  /// @param version 配置版本号
  /// @return 成功 kSuccess
  ErrorCode Save(const ConfigScope& scope,
                 const qtrade::config::v1::EngineConfig& config,
                 std::uint64_t version) override;

 private:
  /// @brief 判断数据库连接是否就绪
  [[nodiscard]] bool IsReady() const;

  qtrade::framework::dao::DbConnectionHolder connection_;  ///< 数据库连接持有者
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_SOCI_CONFIG_REPOSITORY_HPP_
