/// @file      monitor_client.hpp
/// @brief     监控指标客户端
/// @details   A 段仅 Enqueue；Outbound 线程异步上报，远程实现可 stub
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_MONITOR_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_MONITOR_CLIENT_HPP_

#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace qtrade::client::detail {
class OutboundWorker;
}

namespace qtrade::client {

/// @brief 监控指标客户端类
/// @details Counter/Gauge 异步入队，MVP 远程 sink 为 no-op
class MonitorClient {
 public:
  /// @brief 构造监控客户端（未初始化，须调用 Init）
  MonitorClient();

  /// @brief 析构并 Shutdown
  ~MonitorClient();

  MonitorClient(const MonitorClient&) = delete;
  MonitorClient& operator=(const MonitorClient&) = delete;

  /// @brief 初始化监控客户端并启动 Outbound 线程
  /// @param endpoint 监控服务端点地址（MVP 可忽略，保留供远程 gRPC/HTTP 实现）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init(std::string_view endpoint);

  /// @brief 关闭 Outbound 线程并释放资源
  void Shutdown();

  /// @brief 异步入队 Counter 指标（P2，队列满时可丢弃）
  /// @param name 指标名称
  /// @param value 本次递增量
  void Counter(std::string_view name, double value);

  /// @brief 异步入队 Gauge 指标（P2，队列满时可丢弃）
  /// @param name 指标名称
  /// @param value 当前瞬时值
  void Gauge(std::string_view name, double value);

 private:
  std::string endpoint_;                            ///< 监控服务端点（预留）
  bool initialized_ = false;                        ///< 是否已完成 Init
  std::unique_ptr<detail::OutboundWorker> worker_;  ///< Outbound 队列与专用线程
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_MONITOR_CLIENT_HPP_
