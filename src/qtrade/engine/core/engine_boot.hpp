/// @file      engine_boot.hpp
/// @brief     交易引擎进程启动阶段（业务相关）
/// @details   共用阶段见 common/app/process_boot；本文件仅引擎特有步骤。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_BOOT_HPP_
#define QTRADE_ENGINE_ENGINE_BOOT_HPP_

#include <string>

namespace qtrade::engine {

class TradingEngine;

/// @brief 引擎业务启动阶段（供 main 编排调用）
namespace boot {

/// @brief 加载本地引导配置（qtrade_engine.json）；文件缺失时打警告并沿用默认配置
[[nodiscard]] bool LoadBootstrapConfig(TradingEngine& engine, const std::string& config_path);

/// @brief 注册演示策略工厂与默认策略实例
[[nodiscard]] bool RegisterDemoStrategies(TradingEngine& engine);

/// @brief 调用 TradingEngine::Init
[[nodiscard]] bool InitEngine(TradingEngine& engine);

/// @brief 调用 TradingEngine::Start（含演示行情订阅）
[[nodiscard]] bool StartEngine(TradingEngine& engine);

/// @brief 阻塞至停机信号后调用 TradingEngine::Stop
void RunUntilShutdown(TradingEngine& engine);

}  // namespace boot
}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_BOOT_HPP_
