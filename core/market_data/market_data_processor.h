#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "data_source.h"
#include "../event_bus/event_bus.h"
#include "tick_data.h"
#include "bar_data.h"

namespace quant {
namespace core {
namespace market_data {

// 行情处理器
class MarketDataProcessor {
public:
    explicit MarketDataProcessor(core::event_bus::EventBus& event_bus);
    ~MarketDataProcessor();
    
    // 初始化
    bool initialize(const std::string& config_path);
    
    // 加载数据源插件
    bool load_data_source(const std::string& name, const std::string& plugin_path);
    
    // 订阅合约
    void subscribe_instrument(const std::string& data_source, const std::string& instrument);
    
    // 取消订阅
    void unsubscribe_instrument(const std::string& data_source, const std::string& instrument);
    
    // 启动所有数据源
    bool start_all();
    
    // 停止所有数据源
    void stop_all();
    
private:
    // 处理原始行情数据
    void process_raw_tick(const std::string& data_source, const RawTickData& raw_tick);
    
    // 生成K线数据
    void generate_bars(const TickData& tick);
    
    core::event_bus::EventBus& event_bus_;
    std::unordered_map<std::string, std::shared_ptr<IDataSource>> data_sources_;
    std::unordered_map<std::string, std::unordered_map<std::string, TickData>> last_ticks_;
    // 其他成员变量...
};

} // namespace market_data
} // namespace core
} // namespace quant
