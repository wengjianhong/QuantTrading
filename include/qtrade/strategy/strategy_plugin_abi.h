/// @file      strategy_plugin_abi.h
/// @brief     策略动态插件 C ABI 约定
/// @details   每个策略 .so 仅导出发现/创建/销毁符号；发单等能力经 IStrategy 虚函数注入，不进入 C ABI。
///            插件与引擎须使用相同的 IStrategy 布局与编译器 ABI；以 abi_version 做门禁。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_STRATEGY_STRATEGY_PLUGIN_ABI_H_
#define QTRADE_STRATEGY_STRATEGY_PLUGIN_ABI_H_

#ifdef __cplusplus
#include <qtrade/strategy/strategy.hpp>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// 当前引擎期望的策略插件 ABI 版本；不匹配则拒绝加载
#define QTRADE_STRATEGY_ABI_VERSION 1

/// dlsym 符号名
#define QTRADE_STRATEGY_SYM_ABI_VERSION "qtrade_strategy_abi_version"
#define QTRADE_STRATEGY_SYM_PLUGIN_NAME "qtrade_strategy_plugin_name"
#define QTRADE_STRATEGY_SYM_CREATE "qtrade_strategy_create"
#define QTRADE_STRATEGY_SYM_DESTROY "qtrade_strategy_destroy"

/// @brief 返回插件实现的 ABI 版本
int qtrade_strategy_abi_version(void);

/// @brief 返回插件名（与 EngineConfig.strategies[].plugin 对齐，如 "example"）
const char* qtrade_strategy_plugin_name(void);

#ifdef __cplusplus
/// @brief 创建策略实例（无宿主参数；发单经 IStrategy::SetOrderSender 注入）
qtrade::strategy::IStrategy* qtrade_strategy_create(void);

/// @brief 销毁由 qtrade_strategy_create 返回的实例
void qtrade_strategy_destroy(qtrade::strategy::IStrategy* strategy);
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // QTRADE_STRATEGY_STRATEGY_PLUGIN_ABI_H_
