/// @file      db_connection.cpp
/// @brief     DbConnectionHolder 实现
/// @author    wengjianhong
/// @date      2026-07-09
/// @copyright CC BY-NC-SA 4.0
#include "common/database/db_connection.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::framework::dao {

DbConnectionHolder::DbConnectionHolder(const qtrade::common::DatabaseOptions& options) {
  // 1. 连接池模式：打开连接池并获取一条连接
  if (options.pool.has_value()) {
    pool_ = cpp_utils::database::CreateConnectionPool();
    if (const auto rc = pool_->Open(*options.pool); rc != cpp_utils::database::Error::kSuccess) {
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
  auto owned = std::make_unique<cpp_utils::database::Connection>(options.connection);
  if (const auto rc = owned->Connect(); rc != cpp_utils::database::Error::kSuccess) {
    spdlog::error("[DbConnectionHolder] connect failed: {}", owned->LastError());
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

bool DbConnectionHolder::IsReady() const { return connection_ != nullptr && connection_->IsConnected(); }

cpp_utils::database::IConnection* DbConnectionHolder::Connection() const { return connection_.get(); }

}  // namespace qtrade::framework::dao
