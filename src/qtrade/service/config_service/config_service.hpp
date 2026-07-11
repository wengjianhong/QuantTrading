/// @file      config_service.hpp
/// @brief     配置中心支撑服务（进程级生命周期控制器）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_SERVICE_HPP_
#define QTRADE_SERVICE_CONFIG_SERVICE_HPP_

#include "qtrade_framework/support/support_service_impl.hpp"
#include "qtrade/service/config_service/config_grpc_async_handler.hpp"
#include "qtrade/service/config_service/repository/config_repository.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.grpc.pb.h>

#include <string>

namespace qtrade::service {

/// @brief 配置中心支撑服务
class ConfigService final
  : public qtrade::common::support::SupportServiceImpl<qtrade::config::v1::ConfigService::AsyncService,
                                                       ConfigGrpcAsyncHandler> {
 public:
  ConfigService();

  ErrorCode Initialize(const std::string& config_path) override;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_SERVICE_HPP_
