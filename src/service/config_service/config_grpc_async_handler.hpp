/// @file      config_grpc_async_handler.hpp
/// @brief     ConfigService Async + CQ RPC 处理器
/// @author    wengjianhong
/// @date      2026-06-28
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_
#define QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_

#include "service/config_service/repository/config_repository.hpp"

#include <qtrade/proto/config/v1/config.grpc.pb.h>

#include <memory>

namespace grpc {
class ServerCompletionQueue;
}

namespace qtrade::service {

/// @brief 管理 GetConfig / WatchConfig 的 Async CallData 生命周期
class ConfigGrpcAsyncHandler {
 public:
  using RepositoryT = IConfigRepository;

  /// @brief 构造 RPC 处理器
  ConfigGrpcAsyncHandler();

  /// @brief 析构并调用 Shutdown
  ~ConfigGrpcAsyncHandler();

  ConfigGrpcAsyncHandler(const ConfigGrpcAsyncHandler&) = delete;
  ConfigGrpcAsyncHandler& operator=(const ConfigGrpcAsyncHandler&) = delete;

  /// @brief 绑定 AsyncService、CQ 与数据库仓储
  /// @param async_service gRPC 异步服务指针
  /// @param cq 服务端 CompletionQueue
  /// @param repository 数据库仓储
  void Init(qtrade::config::v1::ConfigService::AsyncService* async_service,
            grpc::ServerCompletionQueue* cq,
            std::shared_ptr<IConfigRepository> repository);

  /// @brief 预投递 RPC 接收
  void Start();

  /// @brief 停止新推送（Shutdown 前调用）
  void Shutdown();

  /// @brief 预投递下一个 GetConfig 异步接收
  void SpawnGetConfig();

  /// @brief 预投递下一个 WatchConfig 异步接收
  void SpawnWatchConfig();

  /// @brief 从数据库查询指定作用域配置快照
  /// @param scope 租户与引擎实例
  /// @return 配置快照
  [[nodiscard]] qtrade::config::v1::ConfigSnapshot QuerySnapshot(const ConfigScope& scope) const;

  /// @brief 获取数据库仓储
  /// @return 仓储共享指针
  [[nodiscard]] std::shared_ptr<IConfigRepository> Repository() const { return repository_; }

  /// @brief 获取 CompletionQueue
  /// @return 服务端 CQ 指针
  [[nodiscard]] grpc::ServerCompletionQueue* CompletionQueue() const { return cq_; }

  /// @brief 获取 gRPC 异步服务
  /// @return AsyncService 指针
  [[nodiscard]] qtrade::config::v1::ConfigService::AsyncService* AsyncService() const { return async_service_; }

 private:
  qtrade::config::v1::ConfigService::AsyncService* async_service_ = nullptr;  ///< gRPC 异步服务
  grpc::ServerCompletionQueue* cq_ = nullptr;                                  ///< 服务端 CQ
  std::shared_ptr<IConfigRepository> repository_;                              ///< 数据库仓储
  bool started_ = false;                                                       ///< 是否已启动
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_GRPC_ASYNC_HANDLER_HPP_
