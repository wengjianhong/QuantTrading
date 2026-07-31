/// @file      account_service_define.hpp
/// @brief     账户服务常量定义
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_ACCOUNT_SERVICE_DEFINE_HPP_
#define QTRADE_SERVICE_ACCOUNT_SERVICE_DEFINE_HPP_

#include <string>

namespace qtrade::service::account_service {
/// @brief 账户服务名称
const std::string kServiceName = "qtrade_account_service";
/// @brief 账户服务默认监听端口
constexpr const int kDefaultPort = 50052;
/// @brief 账户服务默认日志目录
const std::string kDefaultLogDir = "logs";
/// @brief 账户服务默认日志文件名
const std::string kDefaultLogFilename = "qtrade_account_service.log";

}  // namespace qtrade::service::account_service
#endif  // QTRADE_SERVICE_ACCOUNT_SERVICE_DEFINE_HPP_
