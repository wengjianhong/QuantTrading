#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../oms/order.h"
#include "../oms/trade.h"
#include "execution_result.h"

namespace quant {
namespace core {
namespace ems {

// 执行适配器接口
class IExecutionAdapter {
public:
    virtual ~IExecutionAdapter() = default;
    
    // 连接交易所
    virtual bool connect() = 0;
    
    // 断开连接
    virtual void disconnect() = 0;
    
    // 发送订单
    virtual ExecutionResult send_order(const oms::Order& order) = 0;
    
    // 取消订单
    virtual ExecutionResult cancel_order(const std::string& order_id) = 0;
    
    // 修改订单
    virtual ExecutionResult modify_order(const oms::Order& order) = 0;
    
    // 查询订单
    virtual std::vector<oms::Order> query_orders(const std::string& instrument = "") = 0;
    
    // 查询成交
    virtual std::vector<oms::Trade> query_trades(const std::string& instrument = "") = 0;
    
    // 获取适配器ID
    virtual std::string id() const = 0;
    
    // 获取支持的合约
    virtual std::vector<std::string> supported_instruments() const = 0;
    
    // 设置订单状态回调
    using OrderCallback = std::function<void(const oms::Order&)>;
    virtual void set_order_callback(OrderCallback callback) = 0;
    
    // 设置成交回调
    using TradeCallback = std::function<void(const oms::Trade&)>;
    virtual void set_trade_callback(TradeCallback callback) = 0;
};

// 执行适配器工厂
class ExecutionAdapterFactory {
public:
    // 创建适配器实例
    static std::shared_ptr<IExecutionAdapter> create_adapter(
        const std::string& type, const std::unordered_map<std::string, std::string>& config);
    
    // 加载适配器插件
    static bool load_adapter_plugin(const std::string& plugin_path);
};

} // namespace ems
} // namespace core
} // namespace quant
