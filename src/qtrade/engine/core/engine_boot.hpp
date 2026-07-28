/// @file      engine_boot.hpp
/// @brief     交易引擎进程启动阶段（业务相关）
/// @details   共用阶段见 common/app/process_boot；本文件仅引擎特有步骤。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_BOOT_HPP_
#define QTRADE_ENGINE_ENGINE_BOOT_HPP_

#include "qtrade/common/boot/process_boot.hpp"

namespace qtrade::engine {

class TradingEngine;

/// @brief 引擎业务启动阶段（供 main 编排调用）
namespace boot {

/// @brief 注册策略工厂与策略实例
/// @param engine 交易引擎
/// @return 是否成功
[[nodiscard]] bool RegisterStrategies(TradingEngine& engine);

/// @brief 调用 TradingEngine::Init
/// @param engine 交易引擎
/// @param options 程序选项
/// @return 是否成功
[[nodiscard]] bool InitEngine(TradingEngine& engine, const qtrade::common::process_boot::ProgramOptions& options);

/// @brief 调用 TradingEngine::Start（含演示行情订阅）
/// @param engine 交易引擎
/// @return 是否成功
[[nodiscard]] bool StartEngine(TradingEngine& engine);

/// @brief 阻塞至停机信号后调用 TradingEngine::Stop
/// @param engine 交易引擎
void RunUntilShutdown(TradingEngine& engine);

}  // namespace boot
}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_BOOT_HPP_
