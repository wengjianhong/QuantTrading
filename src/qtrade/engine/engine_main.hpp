/// @file      engine_main.hpp
/// @brief     交易引擎进程入口封装
/// @author    wengjianhong
/// @date      2026-07-16
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_MAIN_HPP_
#define QTRADE_ENGINE_ENGINE_MAIN_HPP_

namespace qtrade::engine {

/// @brief 运行交易引擎独立进程
/// @param argc main 传入的参数个数
/// @param argv main 传入的参数数组
/// @return 正常退出返回 EXIT_SUCCESS，参数、初始化或启动失败返回 EXIT_FAILURE
int RunTradingEngineMain(int argc, char** argv);

}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_MAIN_HPP_
