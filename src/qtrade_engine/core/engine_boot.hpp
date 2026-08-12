/// @file      engine_boot.hpp
/// @brief     交易引擎进程启动阶段（业务相关；进程入口私有编排）
/// @details   共用阶段见 common/boot/process_boot。引导 JSON 仅供进程入口使用，不进入 IEngine。
///            编排对象为 IEngine，避免进程入口依赖 TradingEngine 实现细节。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_BOOT_HPP_
#define QTRADE_ENGINE_ENGINE_BOOT_HPP_

#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"

#include <qtrade/engine/engine.hpp>
#include <qtrade/strategy/strategy.hpp>

#include <optional>
#include <string>
#include <vector>

namespace qtrade::engine {

/// @brief 引擎业务启动阶段（供 main 编排调用）
namespace boot {
using qtrade::common::config::QtradeEngineBootstrapConfig;

/// @brief 加载并解析引擎引导配置
/// @param options 程序选项（含 config 路径）
/// @return 解析结果；失败返回 nullopt
[[nodiscard]] std::optional<QtradeEngineBootstrapConfig> LoadBootstrapConfig(
  const qtrade::common::process_boot::ProgramOptions& options);

/// @brief 按约定路径为每个策略调用 AddStrategy(config, plugin_so_path)
/// @details 默认 so 路径：`{plugin_dir}/lib{strategy_name}_strategy.so`
/// @param engine 已 Init 的引擎
/// @param plugin_dir 策略 .so 目录
/// @param strategies 策略业务配置列表
/// @return 是否全部成功
[[nodiscard]] bool LoadStrategies(IEngine& engine,
                                  const std::string& plugin_dir,
                                  const std::vector<qtrade::strategy::StrategyConfig>& strategies);

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
