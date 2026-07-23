/// @file      main.cpp
/// @brief     交易账户服务（qtrade_account_service）
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/boot/support_service_boot.hpp"
#include "qtrade/service/account_service/account_service.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  auto options_result = qtrade::common::process_boot::ParseProgramOptions(argc, argv);
  if (options_result.error_code != qtrade::ErrorCode::kSuccess || !options_result.data.has_value()) {
    std::cerr << "[qtrade_account_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  qtrade::service::AccountService service;
  return qtrade::common::support_boot::RunSupportServiceMain(
    options_result.data.value(), "logs", "account-service.log", service);
}
