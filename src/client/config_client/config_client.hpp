/// @file      config_client.hpp
/// @brief     配置管理客户端
/// @details   控制面：gRPC GetConfig 冷启动 + WatchConfig 出站订阅（按版本全量快照）
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace qtrade::config::v1 {
class ConfigSnapshot;
}

namespace qtrade::client {

/// @brief 配置客户端连接参数
struct ConfigClientOptions {
  std::string server_address;         ///< config-service gRPC 地址，如 localhost:50051
  std::string tenant_id = "default";  ///< 租户 ID，用于多租户隔离
  std::string engine_id = "default";  ///< 引擎实例 ID，用于按实例推送
};

/// @brief 配置管理客户端类
/// @details 引擎作为 gRPC Client 出站连接 config-service；不对外提供 gRPC Server
class ConfigClient {
 public:
  /// @brief 收到全量配置快照（GetConfig 与 WatchConfig 均触发）
  using SnapshotHandler = std::function<void(const qtrade::config::v1::ConfigSnapshot& snapshot)>;

  /// @brief 构造配置客户端（未初始化，须调用 Init）
  ConfigClient();

  /// @brief 析构并 Shutdown
  ~ConfigClient();

  ConfigClient(const ConfigClient&) = delete;
  ConfigClient& operator=(const ConfigClient&) = delete;

  /// @brief 建立 gRPC 通道（不拉取配置）
  /// @param options 连接参数（server_address、tenant_id、engine_id）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init(const ConfigClientOptions& options);

  /// @brief 冷启动：GetConfig 拉全量并触发 SnapshotHandler
  /// @return ErrorCode::kSuccess 表示成功；网络失败返回 ErrorCode::kTimeout
  ErrorCode FetchSnapshot();

  /// @brief 启动控制线程：WatchConfig 订阅新版本全量快照
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode StartWatch();

  /// @brief 停止 Watch 线程并释放 gRPC 资源
  void Shutdown();

  /// @brief 注册全量快照回调
  /// @param handler 收到快照时调用（引擎侧替换本地配置视图）
  void SetOnSnapshot(SnapshotHandler handler);

  /// @brief 获取当前已应用的配置版本号
  /// @return 单调递增版本号；未拉取快照时为 0
  [[nodiscard]] std::uint64_t Version() const;

  /// @brief 是否已完成 Init
  [[nodiscard]] bool IsInitialized() const;

 private:
  void ApplySnapshot(const qtrade::config::v1::ConfigSnapshot& snapshot);

  struct Impl;                  ///< 实现细节（隐藏 gRPC 依赖）
  std::unique_ptr<Impl> impl_;  ///< pimpl 实现体
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_CONFIG_CLIENT_HPP_
