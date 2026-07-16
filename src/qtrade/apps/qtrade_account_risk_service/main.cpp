/// @file      main.cpp
/// @brief     账户硬风控服务进程入口
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/app_runner.hpp"
#include "qtrade/service/account_risk_service/account_risk_service.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string config_path;
  if (!qtrade::common::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_account_risk_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }
  qtrade::service::AccountRiskService service;
  return qtrade::common::RunSupportServiceMain(config_path, "logs", "account-risk-service.log", service);
}
