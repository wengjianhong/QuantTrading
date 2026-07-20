/// @file      db_connection_pool_manager.hpp
/// @brief     服务进程数据库连接池的 RAII 管理器
/// @details   仅管理连接池生命周期；每次 Acquire 返回独占连接，析构时自动归还连接池。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_FRAMEWORK_DATABASE_DB_CONNECTION_POOL_MANAGER_HPP_
#define QTRADE_FRAMEWORK_DATABASE_DB_CONNECTION_POOL_MANAGER_HPP_

#include <cpputils/database/config.hpp>
#include <cpputils/database/connection.hpp>
#include <cpputils/database/connection_pool.hpp>

#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace qtrade::framework::dao {

/// @brief 服务进程的多数据库连接池管理器
class DbConnectionPoolManager {
 public:
  /// @brief 创建空的数据库连接池管理器
  DbConnectionPoolManager() = default;

  ~DbConnectionPoolManager();
  DbConnectionPoolManager(const DbConnectionPoolManager&) = delete;
  DbConnectionPoolManager& operator=(const DbConnectionPoolManager&) = delete;

  /// @brief 查询全部连接池是否均已就绪
  [[nodiscard]] bool IsReady() const;
  /// @brief 查询指定连接池是否可借出连接
  [[nodiscard]] bool IsReady(std::string_view database_name) const;

  /// @brief 注册并打开一个具名数据库连接池
  /// @return 名称重复、名称为空或连接池打开失败时返回 false
  [[nodiscard]] bool AddConnectionPool(std::string database_name,
                                       const cpputils::database::ConnectionPoolConfig& options);

  /// @brief 从指定池借出一条请求或事务独占的数据库连接
  /// @param database_name 连接池名称
  /// @return 成功时返回连接所有权；析构时自动归还池；池不存在、耗尽或未就绪时返回 nullptr
  [[nodiscard]] std::unique_ptr<cpputils::database::IConnection> Acquire(std::string_view database_name);

 private:
  std::map<std::string, std::unique_ptr<cpputils::database::IConnectionPool>, std::less<>> pools_;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_FRAMEWORK_DATABASE_DB_CONNECTION_POOL_MANAGER_HPP_
