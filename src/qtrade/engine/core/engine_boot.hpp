/// @file      engine_boot.hpp
/// @brief     交易引擎进程启动阶段（业务相关）
/// @details   共用阶段见 common/app/process_boot；本文件仅引擎特有步骤。
/// @author    wengjianhong
/// @date      2026-07-19
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_ENGINE_BOOT_HPP_
#define QTRADE_ENGINE_ENGINE_BOOT_HPP_

#include "qtrade/common/boot/process_boot.hpp"
#include "qtrade/common/config/qtrade_engine_bootstrap_config.hpp"

#include <optional>

namespace qtrade::engine {

class TradingEngine;

/// @brief 引擎业务启动阶段（供 main 编排调用）
namespace boot {

/// @brief 加载并解析引擎引导配置
/// @param options 程序选项（含 config 路径）
/// @return 解析结果；失败返回 nullopt
[[nodiscard]] std::optional<qtrade::common::config::QtradeEngineBootstrapConfig> LoadBootstrapConfig(
  const qtrade::common::process_boot::ProgramOptions& options);

/// @brief 按 runtime_config_.strategies 从插件目录加载并注册已启用策略实例
/// @param engine 交易引擎（须已 Init 且持有 runtime_config_ / strategy.plugin_dir）
/// @return 是否成功
[[nodiscard]] bool LoadStrategies(TradingEngine& engine);

/// @brief 调用 TradingEngine::Init（bootstrap → clients → runtime_config → modules → adapters）
/// @param engine 交易引擎
/// @param config 已解析的引导配置
/// @return 是否成功
[[nodiscard]] bool InitEngine(TradingEngine& engine, const qtrade::common::config::QtradeEngineBootstrapConfig& config);

/// @brief 调用 TradingEngine::Start（对账、通道、策略/EMS、按配置订阅行情）
/// @param engine 交易引擎
/// @return 是否成功
[[nodiscard]] bool StartEngine(TradingEngine& engine);

/// @brief 阻塞至停机信号后调用 TradingEngine::Stop
/// @param engine 交易引擎
void RunUntilShutdown(TradingEngine& engine);

}  // namespace boot
}  // namespace qtrade::engine

#endif  // QTRADE_ENGINE_ENGINE_BOOT_HPP_
