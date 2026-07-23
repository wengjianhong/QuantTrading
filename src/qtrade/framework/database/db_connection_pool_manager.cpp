/// @file      db_connection_pool_manager.cpp
/// @brief     DbConnectionPoolManager 实现
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/framework/database/db_connection_pool_manager.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace qtrade::framework::dao {

DbConnectionPoolManager::~DbConnectionPoolManager() {
  for (auto& [name, pool] : pools_) {
    (void)name;
    pool->Close();
  }
}

bool DbConnectionPoolManager::IsReady() const {
  if (pools_.empty()) {
    return false;
  }
  for (const auto& [name, pool] : pools_) {
    (void)name;
    if (pool == nullptr || !pool->IsOpen()) {
      return false;
    }
  }
  return true;
}

bool DbConnectionPoolManager::IsReady(std::string_view database_name) const {
  const auto it = pools_.find(database_name);
  return it != pools_.end() && it->second != nullptr && it->second->IsOpen();
}

bool DbConnectionPoolManager::AddConnectionPool(std::string database_name,
                                                const cpputils::database::ConnectionPoolConfig& options) {
  if (database_name.empty() || pools_.contains(database_name)) {
    return false;
  }
  auto pool = cpputils::database::CreateConnectionPool();
  if (pool == nullptr || !pool->Open(options)) {
    spdlog::error("[DbConnectionPoolManager] open pool '{}' failed", database_name);
    return false;
  }
  pools_.emplace(std::move(database_name), std::move(pool));
  return true;
}

std::unique_ptr<cpputils::database::IConnection> DbConnectionPoolManager::Acquire(std::string_view database_name) {
  const auto it = pools_.find(database_name);
  if (it == pools_.end() || it->second == nullptr || !it->second->IsOpen()) {
    return nullptr;
  }
  return it->second->Acquire();
}

}  // namespace qtrade::framework::dao
