/// @file      trading_engine_context.hpp
/// @brief     引擎上下文：进程级基础数据
/// @author    wengjianhong
/// @date      2026-08-25
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_TRADING_ENGINE_CONTEXT_HPP_
#define QTRADE_ENGINE_TRADING_ENGINE_CONTEXT_HPP_

#include <qtrade/common/system/time.hpp>

#include <cstdint>

namespace qtrade::engine {

/// @brief 本进程引擎世代号
/// @details 首次调用时按 Unix 秒固化，之后不变。
/// @return 进程内固定世代号
[[nodiscard]] inline std::uint64_t EngineEpoch() {
  static const std::uint64_t epoch = static_cast<std::uint64_t>(qtrade::common::system::UnixMillisNow() / 1000);
  return epoch;
}

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_TRADING_ENGINE_CONTEXT_HPP_
