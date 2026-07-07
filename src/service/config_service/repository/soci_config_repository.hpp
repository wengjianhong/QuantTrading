/// @file      soci_config_repository.hpp
/// @brief     基于 cpp_utils::database::Connection 的配置仓储实现
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_SOCI_CONFIG_REPOSITORY_HPP_
#define QTRADE_SERVICE_SOCI_CONFIG_REPOSITORY_HPP_

#include "service/config_service/repository/config_repository.hpp"

#include <cpputils/database/database.hpp>

namespace qtrade::service {

/// @brief 基于 SOCI 的配置持久化实现（engine_config 表存 EngineConfig JSON）
class SociConfigRepository final : public IConfigRepository {
 public:
  /// @brief 根据数据库选项建立连接或连接池
  /// @param options 数据库连接选项
  explicit SociConfigRepository(const qtrade::common::DatabaseOptions& options);

  /// @brief 关闭连接并释放连接池
  ~SociConfigRepository() override;

  /// @brief 确保 engine_config 表存在
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode EnsureSchema() override;

  /// @brief 从数据库加载指定作用域 EngineConfig
  ErrorCode Load(const ConfigScope& scope,
                 qtrade::config::v1::EngineConfig& config,
                 std::uint64_t& version) override;

  /// @brief 将 EngineConfig 写入数据库
  ErrorCode Save(const ConfigScope& scope,
                 const qtrade::config::v1::EngineConfig& config,
                 std::uint64_t version) override;

 private:
  [[nodiscard]] bool IsReady() const;

  std::unique_ptr<cpp_utils::database::IConnectionPool> pool_;
  std::unique_ptr<cpp_utils::database::IConnection> connection_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_SOCI_CONFIG_REPOSITORY_HPP_
