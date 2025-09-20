#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../../core/market_data/raw_tick_data.h"

namespace quant {
namespace plugins {
namespace data_sources {

// 数据源接口
class IDataSource {
public:
    virtual ~IDataSource() = default;
    
    // 初始化
    virtual bool initialize(const std::unordered_map<std::string, std::string>& config) = 0;
    
    // 连接数据源
    virtual bool connect() = 0;
    
    // 断开连接
    virtual void disconnect() = 0;
    
    // 订阅合约
    virtual bool subscribe(const std::vector<std::string>& instruments) = 0;
    
    // 取消订阅
    virtual bool unsubscribe(const std::vector<std::string>& instruments) = 0;
    
    // 获取数据源名称
    virtual std::string name() const = 0;
    
    // 获取支持的合约
    virtual std::vector<std::string> supported_instruments() const = 0;
    
    // 设置行情回调
    using TickCallback = std::function<void(const core::market_data::RawTickData&)>;
    virtual void set_tick_callback(TickCallback callback) = 0;
};

// 插件入口函数声明
extern "C" {
    using CreateDataSourceFunc = std::shared_ptr<IDataSource>(*)();
    using DestroyDataSourceFunc = void(*)(std::shared_ptr<IDataSource>);
}

} // namespace data_sources
} // namespace plugins
} // namespace quant
