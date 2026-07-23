/// @file      main.cpp
/// @brief     配置中心服务（qtrade_config_service）
/// @details   gRPC GetConfig / SubscribeConfig 服务端
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/boot/support_service_boot.hpp"
#include "qtrade/service/config_service/config_service.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  auto options_result = qtrade::common::process_boot::ParseProgramOptions(argc, argv);
  if (options_result.error_code != qtrade::ErrorCode::kSuccess || !options_result.data.has_value()) {
    std::cerr << "[qtrade_config_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  qtrade::service::ConfigService service;
  return qtrade::common::support_boot::RunSupportServiceMain(
    options_result.data.value(), "logs", "config-service.log", service);
}
