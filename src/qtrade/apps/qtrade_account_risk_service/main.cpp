/// @file      main.cpp
/// @brief     账户硬风控服务进程入口
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/boot/support_service_boot.hpp"
#include "qtrade/common/config/qtrade_account_risk_service_bootstrap_config.hpp"
#include "qtrade/common/json/json_util.hpp"
#include "qtrade/service/account_risk_service/account_risk_service.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
  // 解析命令行参数
  auto options_result = qtrade::common::process_boot::ParseProgramOptions(argc, argv);
  if (options_result.error_code != qtrade::ErrorCode::kSuccess || !options_result.data.has_value()) {
    std::cerr << "[qtrade_account_risk_service] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  // 加载配置文件
  const auto config_node = qtrade::common::LoadJsonFile(options_result.data->config_path);
  if (!config_node.has_value()) {
    std::cerr << "[qtrade_account_risk_service] failed to load config file\n";
    return EXIT_FAILURE;
  }

  // 解析配置文件
  using qtrade::common::config::ParseQtradeAccountRiskServiceBootstrapConfig;
  const auto bootstrap_config = ParseQtradeAccountRiskServiceBootstrapConfig(config_node.value());
  if (!bootstrap_config.has_value()) {
    std::cerr << "[qtrade_account_risk_service] failed to parse config file\n";
    return EXIT_FAILURE;
  }

  // 运行服务
  qtrade::service::AccountRiskService service;
  return qtrade::common::support_boot::RunSupportServiceMain(options_result.data.value(),
                                                             bootstrap_config.value().config.log_dir,
                                                             bootstrap_config.value().config.log_filename,
                                                             service);
}
