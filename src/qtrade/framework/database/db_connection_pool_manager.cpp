/// @file      db_connection_pool_manager.cpp
/// @brief     DbConnectionPoolManager 实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <spdlog/spdlog.h>

namespace qtrade::framework::dao {

DbConnectionPoolManager::DbConnectionPoolManager(const cpputils::database::ConnectionPoolConfig& options) {
  pool_ = cpputils::database::CreateConnectionPool();
  if (pool_ == nullptr || !pool_->Open(options)) {
    spdlog::error("[DbConnectionPoolManager] open pool failed");
    pool_.reset();
  }
}

DbConnectionPoolManager::~DbConnectionPoolManager() {
  if (pool_ != nullptr) {
    pool_->Close();
  }
}

bool DbConnectionPoolManager::IsReady() const {
  return pool_ != nullptr && pool_->IsOpen();
}

std::unique_ptr<cpputils::database::IConnection> DbConnectionPoolManager::Acquire() {
  if (!IsReady()) {
    return nullptr;
  }
  return pool_->Acquire();
}

}  // namespace qtrade::framework::dao
