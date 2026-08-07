/// @file      time.hpp
/// @brief     墙钟与单调时钟时间戳工具
/// @author    wengjianhong
/// @date      2026-08-05
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_COMMON_SYSTEM_TIME_HPP_
#define QTRADE_COMMON_SYSTEM_TIME_HPP_

#include <chrono>
#include <cstdint>

namespace qtrade::common::system {

/// @brief 当前 POSIX Unix 毫秒时间戳
/// @note 基于 system_clock，抹平闰秒；会受 NTP 时间回拨影响，禁止用于超时/时间差计算
/// @return Unix 纪元 (1970-01-01 UTC) 以来的毫秒数
[[nodiscard]] inline std::int64_t UnixMillisNow() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
    .count();
}

/// @brief 当前 POSIX Unix 微秒时间戳
/// @note 基于 system_clock，抹平闰秒；会受 NTP 时间回拨影响，禁止用于超时/时间差计算
/// @return Unix 纪元 (1970-01-01 UTC) 以来的微秒数
[[nodiscard]] inline std::int64_t UnixMicrosNow() {
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
    .count();
}

/// @brief 当前 POSIX Unix 纳秒时间戳
/// @note 基于 system_clock，抹平闰秒；会受 NTP 时间回拨影响，禁止用于超时/时间差计算
/// @return Unix 纪元 (1970-01-01 UTC) 以来的纳秒数
[[nodiscard]] inline std::int64_t UnixNanosNow() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
    .count();
}

/// @brief 单调时钟毫秒（用于超时/间隔，非墙钟）
/// @note 基准为系统开机时刻；数值无日历含义，禁止落库、禁止网络对外传输
/// @return steady_clock 开机以来的毫秒计数
[[nodiscard]] inline std::int64_t SteadyMillisNow() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
    .count();
}

/// @brief 单调时钟微秒（用于耗时统计/超时，非墙钟）
/// @note 基准为系统开机时刻；数值无日历含义，禁止落库、禁止网络对外传输
/// @return steady_clock 开机以来的微秒计数
[[nodiscard]] inline std::int64_t SteadyMicrosNow() {
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
    .count();
}

/// @brief 单调时钟纳秒（用于高精度耗时统计，非墙钟）
/// @note 基准为系统开机时刻；数值无日历含义，禁止落库、禁止网络对外传输；虚拟化环境存在噪声
/// @return steady_clock 开机以来的纳秒计数
[[nodiscard]] inline std::int64_t SteadyNanosNow() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
    .count();
}

}  // namespace qtrade::common::system

#endif  // QTRADE_COMMON_SYSTEM_TIME_HPP_
