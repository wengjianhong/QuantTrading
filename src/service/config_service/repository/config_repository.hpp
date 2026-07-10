/// @file      config_repository.hpp
/// @brief     配置持久化抽象（与具体数据库解耦）
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_REPOSITORY_HPP_
#define QTRADE_SERVICE_CONFIG_REPOSITORY_HPP_

#include "common/database/database_options.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.pb.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace qtrade::service {

/// @brief 配置作用域（DB 主键：tenant_id + engine_id；gRPC 请求仅传 engine_id）
struct ConfigScope {
  std::string tenant_id = "default";  ///< 租户 ID
  std::string engine_id = "default";  ///< 引擎实例 ID

  friend auto operator<=>(const ConfigScope&, const ConfigScope&) = default;
};

/// @brief 从 GetConfig 请求构造作用域
[[nodiscard]] ConfigScope MakeConfigScope(const qtrade::config::v1::GetConfigRequest& request);

/// @brief 从 SubscribeConfig 请求构造作用域
[[nodiscard]] ConfigScope MakeConfigScope(const qtrade::config::v1::SubscribeConfigRequest& request);

/// @brief 配置读写仓储接口
class IConfigRepository {
 public:
  virtual ~IConfigRepository() = default;

  virtual ErrorCode EnsureSchema() = 0;

  /// @brief 从数据库加载 EngineConfig
  virtual ErrorCode Load(const ConfigScope& scope,
                         qtrade::config::v1::EngineConfig& config,
                         std::uint64_t& version) = 0;

  /// @brief 将 EngineConfig 写入数据库
  virtual ErrorCode Save(const ConfigScope& scope,
                         const qtrade::config::v1::EngineConfig& config,
                         std::uint64_t version) = 0;
};

[[nodiscard]] std::shared_ptr<IConfigRepository> CreateConfigRepository(const qtrade::common::DatabaseOptions& options);

/// @brief 查库并组装 ConfigSnapshot（gRPC 响应）
[[nodiscard]] qtrade::config::v1::ConfigSnapshot QueryConfigSnapshot(IConfigRepository* repository,
                                                                     const ConfigScope& scope);

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_REPOSITORY_HPP_
