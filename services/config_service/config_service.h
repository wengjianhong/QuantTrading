#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "config_service.grpc.pb.h"

namespace quant {
namespace services {
namespace config {

// 配置服务
class ConfigService {
public:
    ConfigService();
    
    // 启动服务
    void start(int port);
    
    // 停止服务
    void stop();
    
    // 获取配置
    std::unordered_map<std::string, std::string> get_config(
        const std::string& module, const std::string& section);
    
    // 更新配置
    bool update_config(
        const std::string& module, 
        const std::string& section,
        const std::unordered_map<std::string, std::string>& config);
    
private:
    std::unique_ptr<grpc::Server> server_;
    // 其他成员变量...
};

// 配置客户端
class ConfigClient {
public:
    explicit ConfigClient(std::shared_ptr<grpc::Channel> channel);
    
    // 获取配置
    std::unordered_map<std::string, std::string> get_config(
        const std::string& module, const std::string& section);
    
    // 更新配置
    bool update_config(
        const std::string& module, 
        const std::string& section,
        const std::unordered_map<std::string, std::string>& config);
    
private:
    std::unique_ptr<ConfigServiceRpc::Stub> stub_;
};

} // namespace config
} // namespace services
} // namespace quant
