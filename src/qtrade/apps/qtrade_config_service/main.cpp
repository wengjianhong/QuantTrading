/// @file      main.cpp
/// @brief     配置中心服务（qtrade_config_service）
/// @details   gRPC GetConfig / SubscribeConfig 服务端
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/app_runner.hpp"
#include "qtrade/common/app/support_service_main.hpp"
#include "qtrade/service/config_service/config_service.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string config_path;
  if (!qtrade::common::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_config_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  qtrade::service::ConfigService service;
  return qtrade::common::RunSupportServiceMain(config_path, "logs", "config-service.log", service);
}
