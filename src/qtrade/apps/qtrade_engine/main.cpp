/// @file      main.cpp
/// @brief     交易引擎独立进程入口（qtrade_engine）
/// @details   参考 ugos_serv/main.cc：本文件只做分阶段 fail-fast 编排；
///            共用阶段见 process_boot，引擎特有阶段见 engine_boot。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/app/process_boot.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/common/system/systemd_notify.hpp"
#include "qtrade/engine/core/engine_boot.hpp"
#include "qtrade/engine/trading_engine.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  constexpr const char* kServiceName = "qtrade_engine";

  // 1. 阻塞 SIGINT/SIGTERM，避免信号打到已创建线程导致直接退出
  qtrade::common::system::BlockInterruptSignals();

  // 2. 解析命令行参数
  std::string config_path;
  if (!qtrade::common::process_boot::ParseConfigPath(argc, argv, config_path)) {
    std::cerr << "[qtrade_engine] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  // 3. 初始化程序全局环境（日志）
  if (!qtrade::common::process_boot::InitProgramEnv(kServiceName, "logs", "trading-engine.log", config_path)) {
    qtrade::common::system::NotifyError(0, "Failed to initialize program environment");
    return EXIT_FAILURE;
  }

  qtrade::engine::TradingEngine engine;
  // 4. 加载本地引导配置（identity + support_services）
  if (!qtrade::engine::boot::LoadBootstrapConfig(engine, config_path)) {
    qtrade::common::system::NotifyError(0, "Failed to load bootstrap config");
    return EXIT_FAILURE;
  }

  // 5. 注册策略工厂与演示策略实例
  if (!qtrade::engine::boot::RegisterDemoStrategies(engine)) {
    qtrade::common::system::NotifyError(0, "Failed to register demo strategies");
    return EXIT_FAILURE;
  }

  // 6. 初始化引擎（围栏、订单回放、控制面、适配器）
  if (!qtrade::engine::boot::InitEngine(engine)) {
    qtrade::common::system::NotifyError(0, "Failed to initialize engine");
    return EXIT_FAILURE;
  }

  // 7. 启动运行时（柜台对账、事件通道、策略/EMS）
  if (!qtrade::engine::boot::StartEngine(engine)) {
    qtrade::common::system::NotifyError(0, "Failed to start engine");
    return EXIT_FAILURE;
  }
  (void)qtrade::common::system::NotifyReady("qtrade_engine ready");

  // 8. 阻塞运行直至停机信号，并释放引擎资源
  qtrade::engine::boot::RunUntilShutdown(engine);
  return EXIT_SUCCESS;
}
