#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "strategy_base.h"
#include "strategy_factory.h"
#include "../event_bus/event_bus.h"

namespace quant {
namespace core {
namespace strategy {

// 策略引擎
class StrategyEngine {
public:
    explicit StrategyEngine(event_bus::EventBus& event_bus);
    
    // 加载策略配置
    bool load_config(const std::string& config_path);
    
    // 加载策略插件
    bool load_strategy_plugin(const std::string& plugin_path);
    
    // 创建策略实例
    bool create_strategy(const StrategyConfig& config);
    
    // 启动所有策略
    void start_all_strategies();
    
    // 停止所有策略
    void stop_all_strategies();
    
    // 暂停指定策略
    bool pause_strategy(const std::string& strategy_id);
    
    // 恢复指定策略
    bool resume_strategy(const std::string& strategy_id);
    
    // 获取策略状态
    StrategyStatus get_strategy_status(const std::string& strategy_id) const;
    
private:
    event_bus::EventBus& event_bus_;
    StrategyFactory strategy_factory_;
    std::unordered_map<std::string, std::shared_ptr<StrategyBase>> strategies_;
    // 其他成员变量...
};

} // namespace strategy
} // namespace core
} // namespace quant
