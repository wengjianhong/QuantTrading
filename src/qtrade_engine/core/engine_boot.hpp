/// @file      engine_boot.hpp
/// @brief     交易引擎进程启动阶段（业务相关）
/// @details   共用阶段见 common/boot/process_boot；本文件仅引擎特有步骤。
///            编排对象为 IEngine，避免进程入口依赖 TradingEngine 实现细节。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_BOOT_HPP_
#define QTRADE_ENGINE_ENGINE_BOOT_HPP_

#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"

#include <qtrade/engine/engine.hpp>

#include <optional>

namespace qtrade::engine {

/// @brief 引擎业务启动阶段（供 main 编排调用）
namespace boot {
using qtrade::common::config::QtradeEngineBootstrapConfig;

/// @brief 加载并解析引擎引导配置
/// @param options 程序选项（含 config 路径）
/// @return 解析结果；失败返回 nullopt
[[nodiscard]] std::optional<QtradeEngineBootstrapConfig> LoadBootstrapConfig(
  const qtrade::common::process_boot::ProgramOptions& options);

/// @brief 从指定插件目录加载策略（委托 IEngine::LoadStrategiesFromPlugins）
/// @param engine 已 Init 的引擎
/// @param plugin_dir 策略 .so 目录
/// @return 是否成功
[[nodiscard]] bool LoadStrategies(IEngine& engine, const std::string& plugin_dir);

/// @brief 调用 IEngine::Start
/// @param engine 交易引擎
/// @return 是否成功
[[nodiscard]] bool StartEngine(IEngine& engine);

/// @brief 阻塞至停机信号后调用 IEngine::Stop
/// @param engine 交易引擎
void RunUntilShutdown(IEngine& engine);

}  // namespace boot
}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_BOOT_HPP_
