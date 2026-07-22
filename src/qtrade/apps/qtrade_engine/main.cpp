/// @file      main.cpp
/// @brief     交易引擎独立进程入口（qtrade_engine）
/// @details   参考 ugos_serv/main.cc：本文件只做分阶段 fail-fast 编排；
///            共用阶段见 process_boot，引擎特有阶段见 engine_boot。
/// @author    wengjianhong
/// @date      2026-05-19
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/system/signal.hpp"
#include "qtrade/common/system/systemd_notify.hpp"
#include "qtrade/engine/core/engine_boot.hpp"
#include "qtrade/engine/trading_engine.hpp"
#include "qtrade/engine/trading_engine_define.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  // 1. 阻塞 SIGINT/SIGTERM，避免信号打到子线程导致直接退出
  qtrade::common::system::BlockInterruptSignals();

  // 2. 解析命令行参数
  auto options_result = qtrade::common::process_boot::ParseProgramOptions(argc, argv);
  if (options_result.error_code != qtrade::ErrorCode::kSuccess || !options_result.data.has_value()) {
    std::cerr << "[qtrade_engine] missing required argument: --config <path>\n";
    return EXIT_FAILURE;
  }

  // 3. 初始化程序全局环境（日志）
  if (!qtrade::common::process_boot::InitProgramEnvironment(qtrade::engine::kServiceName,
                                                            qtrade::engine::kLogDir,
                                                            qtrade::engine::kLogFilename,
                                                            options_result.data.value())) {
    qtrade::common::system::NotifyError(0, "Failed to initialize program environment");
    return EXIT_FAILURE;
  }

  qtrade::engine::TradingEngine engine;
  // 4. 初始化引擎（围栏、订单回放、控制面、适配器）
  if (!qtrade::engine::boot::InitEngine(engine, options_result.data.value().config_path)) {
    qtrade::common::system::NotifyError(0, "Failed to initialize engine");
    return EXIT_FAILURE;
  }

  // 5. 注册策略工厂与演示策略实例
  if (!qtrade::engine::boot::RegisterStrategies(engine)) {
    qtrade::common::system::NotifyError(0, "Failed to register demo strategies");
    return EXIT_FAILURE;
  }

  // 6. 启动运行时（柜台对账、事件通道、策略/EMS）
  if (!qtrade::engine::boot::StartEngine(engine)) {
    qtrade::common::system::NotifyError(0, "Failed to start engine");
    return EXIT_FAILURE;
  }
  (void)qtrade::common::system::NotifyReady("qtrade_engine ready");

  // 7. 阻塞运行直至停机信号，并释放引擎资源
  qtrade::engine::boot::RunUntilShutdown(engine);
  return EXIT_SUCCESS;
}
