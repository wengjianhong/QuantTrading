/// @file      main.cpp
/// @brief     回测引擎服务（qtrade_backtest_service）
/// @details   独立进程入口，解析配置并启动回测服务主循环
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/app_runner.hpp"

int main(int argc, char** argv) {
  return qtrade::common::RunServiceMain(argc, argv, "qtrade_backtest_service", "backtest-service.log");
}
