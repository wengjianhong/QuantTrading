/// @file      db_connection.cpp
/// @brief     DbConnectionHolder 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "qtrade_framework/common/database/db_connection.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::framework::dao {

DbConnectionHolder::DbConnectionHolder(const qtrade::common::DatabaseOptions& options) {
  // 1. 连接池模式：打开连接池并获取一条连接
  if (options.pool.has_value()) {
    pool_ = cpputils::database::CreateConnectionPool();
    if (!pool_->Open(*options.pool)) {
      spdlog::error("[DbConnectionHolder] open pool failed");
      pool_.reset();
      return;
    }
    connection_ = pool_->Acquire();
    if (!connection_) {
      spdlog::error("[DbConnectionHolder] acquire connection failed");
    }
    return;
  }

  // 2. 直连模式：创建连接并 Connect
  auto owned = cpputils::database::CreateConnection(options.connection);
  if (!owned->Connect()) {
    spdlog::error("[DbConnectionHolder] connect failed: {}", owned->LastError().message);
  }
  connection_ = std::move(owned);
}

DbConnectionHolder::~DbConnectionHolder() {
  connection_.reset();
  if (pool_) {
    pool_->Close();
    pool_.reset();
  }
}

bool DbConnectionHolder::IsReady() const {
  return connection_ != nullptr && connection_->IsConnected();
}

cpputils::database::IConnection* DbConnectionHolder::Connection() const {
  return connection_.get();
}

}  // namespace qtrade::framework::dao
