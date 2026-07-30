/// @file      config_service_define.hpp
/// @brief     配置服务常量定义
/// @author    wengjianhong
/// @date      2026-07-15
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_SERVICE_DEFINE_HPP_
#define QTRADE_SERVICE_CONFIG_SERVICE_DEFINE_HPP_

#include <string>

namespace qtrade::service::config_service {
/// @brief 配置服务名称
const std::string kServiceName = "qtrade_config_service";
/// @brief 配置服务默认监听端口
constexpr const int kDefaultPort = 50051;
/// @brief 配置服务默认日志目录
const std::string kDefaultLogDir = "logs";
/// @brief 配置服务默认日志文件名
const std::string kDefaultLogFilename = "qtrade_config_service.log";

}  // namespace qtrade::service::config_service
#endif  // QTRADE_SERVICE_CONFIG_SERVICE_DEFINE_HPP_
