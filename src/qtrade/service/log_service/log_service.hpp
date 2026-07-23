/// @file      log_service.hpp
/// @brief     日志支撑服务（进程级生命周期，MVP 占位）
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_LOG_SERVICE_HPP_
#define QTRADE_SERVICE_LOG_SERVICE_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/support/support_service.hpp>

#include <condition_variable>
#include <mutex>
#include <string>

namespace qtrade::service {

/// @brief 日志支撑服务（MVP：完成进程生命周期，业务 ingest 后续补齐）
class LogService final : public qtrade::common::support::ISupportService {
 public:
  LogService();
  ~LogService() override;

  LogService(const LogService&) = delete;
  LogService& operator=(const LogService&) = delete;

  ErrorCode Initialize(const std::string& config_path) override;
  ErrorCode Start() override;
  void Stop() override;
  void Wait() override;
  [[nodiscard]] qtrade::common::support::SupportServiceStatus GetStatus() const override;

 private:
  mutable std::mutex mutex_;
  std::condition_variable stop_cv_;
  bool stop_requested_ = false;
  qtrade::common::support::SupportServiceStatus status_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_LOG_SERVICE_HPP_
