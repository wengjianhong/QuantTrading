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

#include <memory>

namespace qtrade::framework::dao {

/// @brief 服务进程的数据库连接池管理器
class DbConnectionPoolManager {
 public:
  /// @brief 创建并打开数据库连接池
  /// @param options 连接池配置
  explicit DbConnectionPoolManager(const cpputils::database::ConnectionPoolConfig& options);

  ~DbConnectionPoolManager();
  DbConnectionPoolManager(const DbConnectionPoolManager&) = delete;
  DbConnectionPoolManager& operator=(const DbConnectionPoolManager&) = delete;

  /// @brief 查询连接池是否可借出连接
  [[nodiscard]] bool IsReady() const;

  /// @brief 借出一条请求或事务独占的数据库连接
  /// @return 成功时返回连接所有权；析构时自动归还池；池耗尽或未就绪时返回 nullptr
  [[nodiscard]] std::unique_ptr<cpputils::database::IConnection> Acquire();

 private:
  std::unique_ptr<cpputils::database::IConnectionPool> pool_;
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_FRAMEWORK_DATABASE_DB_CONNECTION_POOL_MANAGER_HPP_
