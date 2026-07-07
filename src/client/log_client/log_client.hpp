/// @file      log_client.hpp
/// @brief     日志客户端
/// @details   A 段仅 Enqueue；Outbound 线程异步上报，远程实现可 stub
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_CLIENT_LOG_CLIENT_HPP_
#define QTRADE_TRADING_CLIENT_LOG_CLIENT_HPP_

#include "client/common/report_priority.hpp"
#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace qtrade::client::detail {
class OutboundWorker;
}

namespace qtrade::client {

/// @brief 日志客户端类
/// @details Emit 非阻塞；默认 sink 写本地 spdlog，远程 gRPC 可后续接入
class LogClient {
 public:
  /// @brief 构造日志客户端（未初始化，须调用 Init）
  LogClient();

  /// @brief 析构并 Shutdown
  ~LogClient();

  LogClient(const LogClient&) = delete;
  LogClient& operator=(const LogClient&) = delete;

  /// @brief 初始化日志客户端并启动 Outbound 线程
  /// @param topic 日志主题/分类，用于 sink 侧区分来源
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init(std::string_view topic);

  /// @brief 关闭 Outbound 线程并释放资源
  void Shutdown();

  /// @brief 异步入队日志（不阻塞调用方）
  /// @param level 日志级别（error/warn/info/debug/audit 等）
  /// @param message 日志消息正文
  void Emit(std::string_view level, std::string_view message);

  /// @brief 异步入队 P0 审计流水（最高优先级，走 Spool 保底）
  /// @param message 审计流水正文
  void EmitAudit(std::string_view message);

 private:
  /// @brief 将日志级别映射为旁路优先级
  /// @param level 日志级别字符串
  /// @return 对应的 ReportPriority
  static ReportPriority LevelToPriority(std::string_view level);

  std::string topic_;                               ///< 日志主题/分类
  bool initialized_ = false;                        ///< 是否已完成 Init
  std::unique_ptr<detail::OutboundWorker> worker_;  ///< Outbound 队列与专用线程
};

}  // namespace qtrade::client

#endif  // QTRADE_TRADING_CLIENT_LOG_CLIENT_HPP_
