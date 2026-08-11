/// @file      trading_engine_define.hpp
/// @brief     交易引擎定义
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_TRADING_ENGINE_DEFINE_HPP_
#define QTRADE_TRADING_ENGINE_TRADING_ENGINE_DEFINE_HPP_

#include <string>

namespace qtrade::engine {
/// @brief 服务名称
const std::string kServiceName = "qtrade_engine";
/// @brief 日志目录
const std::string kLogDir = "logs";
/// @brief 日志文件名
const std::string kLogFilename = "trading-engine.log";
}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_TRADING_ENGINE_DEFINE_HPP_
