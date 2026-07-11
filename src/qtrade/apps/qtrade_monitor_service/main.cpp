/// @file      main.cpp
/// @brief     业务监控服务（qtrade_monitor_service）
/// @details   独立进程入口，解析配置并启动监控服务主循环
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/app_runner.hpp"

int main(int argc, char** argv) {
  return qtrade::common::RunServiceMain(argc, argv, "qtrade_monitor_service", "monitor-service.log");
}
