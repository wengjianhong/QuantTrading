#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "../event_bus/event_bus.h"
#include "strategy_status.h"
#include "strategy_config.h"
#include "../market_data/tick_data.h"
#include "../oms/order.h"
#include "../oms/trade.h"

namespace quant {
namespace core {
namespace strategy {

// 策略基类
class StrategyBase {
public:
    explicit StrategyBase(const StrategyConfig& config);
    virtual ~StrategyBase() = default;
    
    // 禁止拷贝和移动
    StrategyBase(const StrategyBase&) = delete;
    StrategyBase& operator=(const StrategyBase&) = delete;
    StrategyBase(StrategyBase&&) = delete;
    StrategyBase& operator=(StrategyBase&&) = delete;
    
    // 初始化策略
    virtual bool initialize(event_bus::EventBus& event_bus);
    
    // 启动策略
    virtual void start();
    
    // 暂停策略
    virtual void pause();
    
    // 恢复策略
    virtual void resume();
    
    // 停止策略
    virtual void stop();
    
    // 处理行情事件
    virtual void on_tick(const market_data::TickData& tick) = 0;
    
    // 处理K线事件
    virtual void on_bar(const market_data::BarData& bar) {}
    
    // 处理订单事件
    virtual void on_order(const oms::Order& order) {}
    
    // 处理成交事件
    virtual void on_trade(const oms::Trade& trade) {}
    
    // 获取策略ID
    std::string id() const;
    
    // 获取策略名称
    std::string name() const;
    
    // 获取策略状态
    StrategyStatus status() const;
    
    // 获取策略参数
    std::string get_parameter(const std::string& key) const;
    
    // 设置策略参数
    void set_parameter(const std::string& key, const std::string& value);
    
protected:
    // 发送交易信号
    void send_signal(const std::string& instrument, double price, int volume, bool is_buy, bool is_open);
    
    StrategyConfig config_;
    StrategyStatus status_;
    event_bus::EventBus* event_bus_;
    // 其他成员变量...
};

} // namespace strategy
} // namespace core
} // namespace quant
