/// @file      config_grpc_async_handler.hpp
/// @brief     ConfigService Async + CQ RPC 处理器
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_
#define QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_

#include "qtrade/service/config_service/grpc/config_scope.hpp"
#include "qtrade/framework/database/db_connection.hpp"

#include <qtrade/proto/config/v1/config.grpc.pb.h>

#include <memory>

namespace grpc {
class ServerCompletionQueue;
}

namespace qtrade::service {

/// @brief 管理 GetConfig / SubscribeConfig 的 Async CallTag 生命周期
class ConfigGrpcAsyncHandler {
 public:
  ConfigGrpcAsyncHandler();

  ~ConfigGrpcAsyncHandler();

  ConfigGrpcAsyncHandler(const ConfigGrpcAsyncHandler&) = delete;
  ConfigGrpcAsyncHandler& operator=(const ConfigGrpcAsyncHandler&) = delete;

  /// @brief 绑定 AsyncService、CQ 与数据库连接
  void Init(qtrade::config::v1::ConfigService::AsyncService* async_service,
            grpc::ServerCompletionQueue* cq,
            std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection);

  void Start();
  void Shutdown();

  void SpawnGetConfig();
  void SpawnSubscribeConfig();

  /// @brief 从数据库查询指定作用域配置快照
  [[nodiscard]] qtrade::config::v1::ConfigSnapshot QuerySnapshot(const ConfigScope& scope) const;

  [[nodiscard]] grpc::ServerCompletionQueue* CompletionQueue() const {
    return cq_;
  }

  [[nodiscard]] qtrade::config::v1::ConfigService::AsyncService* AsyncService() const {
    return async_service_;
  }

 private:
  [[nodiscard]] bool DatabaseReady() const {
    return connection_ != nullptr && connection_->IsReady();
  }

  qtrade::config::v1::ConfigService::AsyncService* async_service_ = nullptr;
  grpc::ServerCompletionQueue* cq_ = nullptr;
  std::shared_ptr<qtrade::framework::dao::DbConnectionHolder> connection_;
  bool started_ = false;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_
