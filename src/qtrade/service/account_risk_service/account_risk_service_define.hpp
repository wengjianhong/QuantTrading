/// @file      account_risk_define.hpp
/// @brief     账户硬风控支撑服务常量定义
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_RISK_DEFINE_HPP_
#define QTRADE_SERVICE_ACCOUNT_RISK_DEFINE_HPP_

#include <string>

namespace qtrade::service::account_risk_service {
/// @brief 账户硬风控支撑服务名称
const std::string kServiceName = "qtrade_account_risk_service";
/// @brief 账户硬风控支撑服务默认监听端口
constexpr const int kDefaultPort = 50053;
/// @brief 账户硬风控支撑服务默认日志目录
const std::string kDefaultLogDir = "logs";
/// @brief 账户硬风控支撑服务默认日志文件名
const std::string kDefaultLogFilename = "qtrade_account_risk_service.log";

}  // namespace qtrade::service::account_risk_service
#endif  // QTRADE_SERVICE_ACCOUNT_RISK_DEFINE_HPP_
