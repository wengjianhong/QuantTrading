/// @file      example_strategy_plugin.cpp
/// @brief     ExampleStrategy 的 C ABI 插件导出
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#include "example_strategy.hpp"

#include <qtrade/strategy/strategy_plugin_abi.h>

namespace {

#if defined(__GNUC__) || defined(__clang__)
#define QTRADE_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#define QTRADE_PLUGIN_EXPORT
#endif

}  // namespace

extern "C" {

QTRADE_PLUGIN_EXPORT int qtrade_strategy_abi_version(void) {
  return QTRADE_STRATEGY_ABI_VERSION;
}

QTRADE_PLUGIN_EXPORT const char* qtrade_strategy_plugin_name(void) {
  return "example";
}

QTRADE_PLUGIN_EXPORT qtrade::strategy::IStrategy* qtrade_strategy_create(void) {
  return new qtrade::demo::ExampleStrategy();
}

QTRADE_PLUGIN_EXPORT void qtrade_strategy_destroy(qtrade::strategy::IStrategy* strategy) {
  delete strategy;
}

}  // extern "C"
