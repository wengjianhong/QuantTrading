/// @file      report_priority.hpp
/// @brief     D 段旁路上报优先级
/// @details   与《架构》§8.1 旁路背压策略对齐
/// @author    wengjianhong
/// @date      2026-06-22
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_CLIENT_REPORT_PRIORITY_HPP_
#define QTRADE_CLIENT_REPORT_PRIORITY_HPP_

#include <cstdint>

namespace qtrade::client {

/// @brief 旁路上报优先级（数值越小越优先保留）
enum class ReportPriority : std::uint8_t {
  kP0Audit = 0,     ///< 合规拦截、发单/撤单流水
  kP1Business = 1,  ///< 订单状态、成交副本
  kP2Metrics = 2,   ///< 延迟直方图、队列深度
  kP3Debug = 3,     ///< verbose 日志
};

}  // namespace qtrade::client

#endif  // QTRADE_CLIENT_REPORT_PRIORITY_HPP_
