#include <iostream>
#include <memory>
#include "event_bus/event_bus.h"
#include "market_data/market_data_processor.h"
#include "strategy/strategy_engine.h"
#include "oms/order_manager.h"
#include "ems/execution_manager.h"
#include "account/account_manager.h"
#include "risk/risk_manager.h"
#include "../services/config_service/config_client.h"

int main(int argc, char* argv[]) {
    try {
        // 初始化事件总线
        auto& event_bus = quant::core::event_bus::EventBus::instance();
        
        // 初始化配置客户端
        quant::services::config::ConfigClient config_client(
            grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
        
        // 初始化账户管理器
        quant::core::account::AccountManager account_manager(event_bus, config_client);
        if (!account_manager.initialize()) {
            std::cerr << "Failed to initialize account manager" << std::endl;
            return 1;
        }
        
        // 初始化风险管理器
        quant::core::risk::RiskManager risk_manager(event_bus, config_client, account_manager);
        if (!risk_manager.initialize()) {
            std::cerr << "Failed to initialize risk manager" << std::endl;
            return 1;
        }
        
        // 初始化执行管理器
        quant::core::ems::ExecutionManager execution_manager(event_bus, config_client);
        if (!execution_manager.initialize()) {
            std::cerr << "Failed to initialize execution manager" << std::endl;
            return 1;
        }
        
        // 初始化订单管理器
        quant::core::oms::OrderManager order_manager(
            event_bus, config_client, execution_manager, risk_manager, account_manager);
        if (!order_manager.initialize()) {
            std::cerr << "Failed to initialize order manager" << std::endl;
            return 1;
        }
        
        // 初始化行情处理器
        quant::core::market_data::MarketDataProcessor market_data_processor(event_bus);
        if (!market_data_processor.initialize("config/market_data.json")) {
            std::cerr << "Failed to initialize market data processor" << std::endl;
            return 1;
        }
        
        // 加载数据源
        market_data_processor.load_data_source("CTP", "plugins/data_sources/ctp_data_source.so");
        market_data_processor.load_data_source("Binance", "plugins/data_sources/binance_data_source.so");
        
        // 启动数据源
        if (!market_data_processor.start_all()) {
            std::cerr << "Failed to start data sources" << std::endl;
            return 1;
        }
        
        // 初始化策略引擎
        quant::core::strategy::StrategyEngine strategy_engine(event_bus);
        if (!strategy_engine.load_config("config/strategies.json")) {
            std::cerr << "Failed to load strategy config" << std::endl;
            return 1;
        }
        
        // 加载策略插件
        strategy_engine.load_strategy_plugin("plugins/strategies/trend_following.so");
        strategy_engine.load_strategy_plugin("plugins/strategies/arbitrage.so");
        
        // 创建策略实例
        // ...
        
        // 启动所有策略
        strategy_engine.start_all_strategies();
        
        // 等待用户输入退出
        std::cout << "Quant trading system is running. Press Enter to exit..." << std::endl;
        std::cin.get();
        
        // 停止策略
        strategy_engine.stop_all_strategies();
        
        // 停止数据源
        market_data_processor.stop_all();
        
        std::cout << "System exited normally" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "System exception: " << e.what() << std::endl;
        return 1;
    }
}
