/// @file      db_connection.hpp
/// @brief     数据库连接/连接池持有
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_DATABASE_DB_CONNECTION_HPP_
#define QTRADE_COMMON_DATABASE_DB_CONNECTION_HPP_

#include "qtrade/framework/database/database_options.hpp"

#include <cpputils/database/connection.hpp>
#include <cpputils/database/connection_pool.hpp>

#include <memory>

namespace qtrade::framework::dao {

/// @brief 数据库连接或连接池的 RAII 持有者
/// @details 根据 DatabaseOptions 选择直连或连接池模式；析构时释放连接并关闭连接池
class DbConnectionHolder {
 public:
  /// @brief 按配置建立数据库连接或连接池
  /// @param options 数据库连接选项（含 pool 配置时走连接池）
  explicit DbConnectionHolder(const qtrade::common::DatabaseOptions& options);

  /// @brief 释放连接并关闭连接池
  ~DbConnectionHolder();

  DbConnectionHolder(const DbConnectionHolder&) = delete;
  DbConnectionHolder& operator=(const DbConnectionHolder&) = delete;

  /// @brief 判断连接是否可用
  /// @return 连接非空且已连接时返回 true
  [[nodiscard]] bool IsReady() const;

  /// @brief 获取底层数据库连接指针
  /// @return 当前持有的 IConnection 指针；未就绪时可能为 nullptr
  [[nodiscard]] cpputils::database::IConnection* Connection() const;

 private:
  std::unique_ptr<cpputils::database::IConnectionPool> pool_;    ///< 连接池（pool 模式时使用）
  std::unique_ptr<cpputils::database::IConnection> connection_;  ///< 当前持有的连接
};

}  // namespace qtrade::framework::dao

#endif  // QTRADE_COMMON_DATABASE_DB_CONNECTION_HPP_
