/// @file      config_service.hpp
/// @brief     配置中心支撑服务（进程级生命周期控制器）
/// @author    wengjianhong
/// @date      2026-07-08
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_SERVICE_CONFIG_SERVICE_HPP_
#define QTRADE_SERVICE_CONFIG_SERVICE_HPP_

#include "qtrade/dao/dao_manager.hpp"
#include "qtrade/framework/support/support_async_service_impl.hpp"
#include "qtrade/service/config_service/grpc/config_grpc_async_handler.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.grpc.pb.h>

#include <memory>
#include <string>

namespace qtrade::service {

/// @brief 配置中心支撑服务（异步 gRPC，含 SubscribeConfig Streaming）
class ConfigService final
  : public qtrade::common::support::SupportAsyncServiceImpl<qtrade::config::v1::ConfigService::AsyncService,
                                                            ConfigGrpcAsyncHandler> {
 public:
  ConfigService();

  ErrorCode Initialize(const std::string& config_path) override;

 protected:
  /// @brief 向 Handler 注入连接与 DaoManager
  void InitHandler() override;

 private:
  /// 本进程 DaoManager（Initialize 创建，供查询路径使用）
  std::shared_ptr<qtrade::framework::dao::DaoManager> dao_mgr_;
};

}  // namespace qtrade::service

#endif  // QTRADE_SERVICE_CONFIG_SERVICE_HPP_
