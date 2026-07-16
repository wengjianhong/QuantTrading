/// @file      main.cpp
/// @brief     交易引擎独立进程入口（qtrade_engine）
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/engine_main.hpp"

int main(int argc, char** argv) {
  return qtrade::engine::RunTradingEngineMain(argc, argv);
}
