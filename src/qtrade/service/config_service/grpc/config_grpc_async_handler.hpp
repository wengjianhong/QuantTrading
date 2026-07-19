/// @file      config_grpc_async_handler.hpp
/// @brief     ConfigService Async + CQ RPC 处理器
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_
#define QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_

#include "qtrade/dao/dao_manager.hpp"
#include "qtrade/framework/database/db_connection_pool_manager.hpp"
#include "qtrade/service/config_service/grpc/config_scope.hpp"

#include <qtrade/proto/config/v1/config.grpc.pb.h>

#include <memory>

namespace qtrade::service {

/// @brief 管理 GetEngineConfig / SubscribeEngineConfig 的 Async CallTag 生命周期
class ConfigGrpcAsyncHandler {
 public:
  ConfigGrpcAsyncHandler();

  ~ConfigGrpcAsyncHandler();

  ConfigGrpcAsyncHandler(const ConfigGrpcAsyncHandler&) = delete;
  ConfigGrpcAsyncHandler& operator=(const ConfigGrpcAsyncHandler&) = delete;

  /// @brief 绑定 AsyncService、CQ、数据库连接与 DaoManager
  void Init(qtrade::config::v1::ConfigService::AsyncService* async_service,
            grpc::ServerCompletionQueue* cq,
            std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection,
            std::shared_ptr<qtrade::framework::dao::DaoManager> dao = nullptr);

  void Start();
  void Shutdown();

  void SpawnGetEngineConfig();
  void SpawnSubscribeEngineConfig();

  /// @brief 从数据库查询指定作用域完整引擎配置
  [[nodiscard]] qtrade::config::v1::EngineConfig QueryConfig(const ConfigScope& scope) const;

  [[nodiscard]] grpc::ServerCompletionQueue* CompletionQueue() const {
    return cq_;
  }

  [[nodiscard]] qtrade::config::v1::ConfigService::AsyncService* AsyncService() const {
    return async_service_;
  }

 private:
  [[nodiscard]] bool DatabaseReady() const {
    return connection_pool_mgr_ != nullptr && connection_pool_mgr_->IsReady() && dao_mgr_ != nullptr;
  }

  /// 是否已启动
  bool started_ = false;
  /// AsyncService
  qtrade::config::v1::ConfigService::AsyncService* async_service_ = nullptr;
  /// CQ
  grpc::ServerCompletionQueue* cq_ = nullptr;

  /// 本进程 DaoManager
  std::shared_ptr<qtrade::framework::dao::DaoManager> dao_mgr_;
  /// 数据库连接池管理器
  std::shared_ptr<qtrade::framework::dao::DbConnectionPoolManager> connection_pool_mgr_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_
