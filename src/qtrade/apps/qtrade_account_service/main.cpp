/// @file      main.cpp
/// @brief     交易账户服务（qtrade_account_service）
/// @author    wengjianhong
/// @date      2026-07-03
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/process_boot.hpp"
#include "qtrade/common/app/support_boot.hpp"
#include "qtrade/service/account_service/account_service.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string config_path;
  if (!qtrade::common::process_boot::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_account_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  qtrade::service::AccountService service;
  return qtrade::common::support_boot::RunSupportServiceMain(config_path, "logs", "account-service.log", service);
}
