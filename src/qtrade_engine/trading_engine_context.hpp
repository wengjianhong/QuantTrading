/// @file      trading_engine_context.hpp
/// @brief     引擎上下文：引擎启动时注入的进程级基础数据
/// @details   世代号、时区等不属于任一业务模块，由本头文件以 inline 函数集中提供。
///            墙钟/单调时钟请用 qtrade/common/system/time.hpp，本文件不重复封装。
///            世代号在首次 EngineEpoch() 时按 Unix 秒固化；时区由组合根 Set。
///            各模块只读查询，不得再各自缓存副本。
/// @author    wengjianhong
/// @date      2026-08-25
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_TRADING_ENGINE_CONTEXT_HPP_
#define QTRADE_ENGINE_TRADING_ENGINE_CONTEXT_HPP_

#include <qtrade/common/system/time.hpp>

#include <cstdint>

namespace qtrade::engine {

// ---------------------------------------------------------------------------
// 世代号（写入 order_id）
// ---------------------------------------------------------------------------

/// @brief 本进程引擎世代号
/// @details 首次调用时按 Unix 秒固化，之后不变。
/// @return 进程内固定世代号
[[nodiscard]] inline std::uint64_t EngineEpoch() {
  static const std::uint64_t epoch = static_cast<std::uint64_t>(qtrade::common::system::UnixMillisNow() / 1000);
  return epoch;
}

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_TRADING_ENGINE_CONTEXT_HPP_
