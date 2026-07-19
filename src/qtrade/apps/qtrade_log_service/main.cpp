/// @file      main.cpp
/// @brief     日志分析服务（qtrade_log_service）
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/process_boot.hpp"
#include "qtrade/common/app/support_boot.hpp"
#include "qtrade/service/log_service/log_service.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string config_path;
  if (!qtrade::common::process_boot::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_log_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  qtrade::service::LogService service;
  return qtrade::common::support_boot::RunSupportServiceMain(config_path, "logs", "log-service.log", service);
}
