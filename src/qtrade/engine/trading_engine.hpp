/// @file      trading_engine.hpp
/// @brief     交易引擎主入口
/// @details   整合各个子模块（账户、风控、订单、持仓等），协调整个交易流程
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
#define QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
#include "qtrade/client/account_risk_client/account_risk_client.hpp"
#include "qtrade/client/config_client/config_client.hpp"
#include "qtrade/client/log_client/log_client.hpp"
#include "qtrade/client/monitor_client/monitor_client.hpp"
#include "qtrade/common/config/qtrade_engine_config.hpp"
#include "qtrade/engine/account/account_manager.hpp"
#include "qtrade/engine/cms/compliance_manager.hpp"
#include "qtrade/engine/ems/execution_manager.hpp"
#include "qtrade/engine/event_bus/event_lanes.hpp"
#include "qtrade/engine/normalizer/quote_normalizer.hpp"
#include "qtrade/engine/normalizer/trader_normalizer.hpp"
#include "qtrade/engine/oms/order_manager.hpp"
#include "qtrade/engine/order_pipeline.hpp"
#include "qtrade/engine/position/position_manager.hpp"
#include "qtrade/engine/risk/risk_manager.hpp"
#include "qtrade/engine/strategy/strategy_engine.hpp"

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/proto/config/v1/config.pb.h>

namespace qtrade::engine {

/// @brief 交易引擎：单进程封闭运行，整合行情、策略、OMS、EMS 等模块
class TradingEngine {
 public:
  /// @brief 构造交易引擎（未初始化，须 Init 后 Start）
  TradingEngine();

  /// @brief 析构并 Stop
  ~TradingEngine();

  TradingEngine(const TradingEngine&) = delete;
  TradingEngine& operator=(const TradingEngine&) = delete;

  /// @brief 初始化控制面 gRPC 与 D 段 client（须在 Start 之前调用）
  /// @param config 进程引导配置（config/account 地址、tenant、log/monitor 等）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 使用已加载的 config_ 初始化（须先 ReloadFromJson 或手动设置 config）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Init();

  /// @brief 从本地 JSON 文件加载/重载引擎配置（如 config/qtrade_engine.json）
  /// @param json_path 配置文件路径
  /// @return ErrorCode::kSuccess 表示成功；文件不存在返回 ErrorCode::kNotFound
  ErrorCode ReloadFromJson(const std::string& json_path);

  /// @brief 返回当前进程引导配置快照
  /// @return 配置只读引用
  [[nodiscard]] const qtrade::common::config::QtradeEngineConfig& GetConfig() const {
    return config_;
  }

  /// @brief 停止所有子模块与 client
  /// @return ErrorCode::kSuccess 表示成功；未运行返回 ErrorCode::kSystemError
  ErrorCode Stop();

  /// @brief 启动所有子模块（须先 Init）
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode Start();

  /// @brief 引擎是否处于运行中
  /// @return true 表示已 Start
  [[nodiscard]] bool IsRunning() const;

  /// @brief 获取事件通道门面（Lane-M + Lane-R）
  /// @return 事件通道引用
  event_bus::EventLanes& GetEventLanes() {
    return event_lanes_;
  }

  /// @brief 获取行情标准化模块引用
  /// @return QuoteNormalizer 引用
  normalizer::QuoteNormalizer& GetQuoteNormalizer() {
    return quote_normalizer_;
  }

  /// @brief 获取交易标准化模块引用
  /// @return TraderNormalizer 引用
  normalizer::TraderNormalizer& GetTraderNormalizer() {
    return trader_normalizer_;
  }

  /// @brief 获取策略引擎引用
  /// @return StrategyEngine 引用
  strategy::StrategyEngine& GetStrategyEngine() {
    return strategy_engine_;
  }

  /// @brief 获取日志客户端引用
  /// @return LogClient 引用
  client::LogClient& GetLogClient() {
    return log_client_;
  }

  /// @brief 获取监控客户端引用
  /// @return MonitorClient 引用
  client::MonitorClient& GetMonitorClient() {
    return monitor_client_;
  }

  /// @brief 获取配置客户端引用
  /// @return ConfigClient 引用
  client::ConfigClient& GetConfigClient() {
    return config_client_;
  }

  /// @brief 将策略请求送入 CMS → Risk → OMS → EMS 发单链
  /// @param request 策略下单请求
  /// @return ErrorCode::kSuccess 表示成功进入 EMS
  ErrorCode SubmitOrder(const qtrade_sdk::trader::OrderRequest& request);

  /// @brief 获取订单管理模块引用
  /// @return OrderManager 引用
  oms::OrderManager& GetOrderManager() {
    return order_manager_;
  }

  /// @brief 获取账户管理模块引用
  /// @return AccountManager 引用
  account::AccountManager& GetAccountManager() {
    return account_manager_;
  }

  /// @brief 获取持仓管理模块引用
  /// @return PositionManager 引用
  position::PositionManager& GetPositionManager() {
    return position_manager_;
  }

 private:
  /// @brief 初始化并连接 config_client（GetConfig + SubscribeConfig）
  /// @param config 进程引导配置
  /// @return ErrorCode::kSuccess 表示成功
  ErrorCode InitConfigClient(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 按配置初始化账户硬风控客户端并注入 OrderPipeline
  /// @param config 进程引导配置
  /// @return ErrorCode::kSuccess 表示成功；启用但地址非法时返回错误码
  ErrorCode InitAccountRiskClient(const qtrade::common::config::QtradeEngineConfig& config);

  /// @brief 配置全量快照回调：应用 EngineConfig 并旁路日志
  /// @param snapshot 含 version 与 engine 的全量快照
  void OnConfigSnapshot(const qtrade::config::v1::ConfigSnapshot& snapshot);

  /// 是否已完成 Init
  bool initialized_ = false;
  /// 是否已 Start
  bool running_ = false;
  /// 进程引导配置（qtrade_engine.json）
  qtrade::common::config::QtradeEngineConfig config_;
  /// config-service 下发的业务配置
  qtrade::config::v1::EngineConfig runtime_config_;
  /// Lane-M / Lane-R 事件通道
  event_bus::EventLanes event_lanes_;
  /// 策略引擎
  strategy::StrategyEngine strategy_engine_;
  /// 行情标准化
  normalizer::QuoteNormalizer quote_normalizer_;
  /// 交易标准化
  normalizer::TraderNormalizer trader_normalizer_;
  /// 合规模块
  cms::ComplianceManager compliance_;
  /// 执行管理模块
  ems::ExecutionManager execution_manager_;
  /// 订单管理模块
  oms::OrderManager order_manager_;
  /// 账户管理模块
  account::AccountManager account_manager_;
  /// 持仓管理模块
  position::PositionManager position_manager_;
  /// 风险管理模块
  risk::RiskManager risk_manager_;
  /// 发单准入流水线
  OrderPipeline order_pipeline_{compliance_, risk_manager_, order_manager_, execution_manager_};
  /// 控制面 gRPC 客户端
  client::ConfigClient config_client_;
  /// E 段账户硬风控客户端
  client::AccountRiskClient account_risk_client_;
  /// D 段日志旁路客户端
  client::LogClient log_client_;
  /// D 段监控旁路客户端
  client::MonitorClient monitor_client_;
};

}  // namespace qtrade::engine

#endif  // QTRADE_TRADING_ENGINE_TRADING_ENGINE_HPP_
