/// @file      bridge_plugin_abi.h
/// @brief     桥接动态插件 C ABI 约定
/// @warning   本头文件仅用于 C++ 编译，禁止由 C 编译器引入
/// @details   每个桥接 .so 导出 abi_version / plugin_name，以及所需的 create/destroy。
///            同一 so 可同时导出 config / account / account_risk 多套符号（集成包），
///            也可只导出其中一部分。create 仅构造；调用方须再调 I*Bridge::Start()。
///            插件与引擎须使用相同的 I*Bridge 布局与编译器 ABI；以 abi_version 做门禁。
///
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_BRIDGE_BRIDGE_PLUGIN_ABI_H_
#define QTRADE_BRIDGE_BRIDGE_PLUGIN_ABI_H_

#include <qtrade/bridge/account_bridge.hpp>
#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/bridge/config_bridge.hpp>

extern "C" {
/// 当前引擎期望的桥接插件 ABI 版本；不匹配则拒绝加载
#define QTRADE_BRIDGE_ABI_VERSION 1

/// dlsym 符号名
#define QTRADE_BRIDGE_SYM_ABI_VERSION "qtrade_bridge_abi_version"
#define QTRADE_BRIDGE_SYM_PLUGIN_NAME "qtrade_bridge_plugin_name"

#define QTRADE_BRIDGE_SYM_CREATE_CONFIG "qtrade_create_config_bridge"
#define QTRADE_BRIDGE_SYM_DESTROY_CONFIG "qtrade_destroy_config_bridge"

#define QTRADE_BRIDGE_SYM_CREATE_ACCOUNT "qtrade_create_account_bridge"
#define QTRADE_BRIDGE_SYM_DESTROY_ACCOUNT "qtrade_destroy_account_bridge"

#define QTRADE_BRIDGE_SYM_CREATE_ACCOUNT_RISK "qtrade_create_account_risk_bridge"
#define QTRADE_BRIDGE_SYM_DESTROY_ACCOUNT_RISK "qtrade_destroy_account_risk_bridge"

/// @brief 返回插件实现的 ABI 版本
int qtrade_bridge_abi_version(void);

/// @brief 返回桥接插件名称（用于配置引用，如 "grpc"）
const char* qtrade_bridge_plugin_name(void);

/// @brief 创建配置桥接实例
/// @param options_json 端点 JSON（host/port/…）；config 另需 engine_id 字段；可为 nullptr
/// @return 实例指针；失败返回 nullptr。调用方随后应 Start()，销毁须用对应 destroy
qtrade::config::IConfigBridge* qtrade_create_config_bridge(const char* options_json);

/// @brief 销毁由 qtrade_create_config_bridge 创建的实例（内部应 Stop）
void qtrade_destroy_config_bridge(qtrade::config::IConfigBridge* bridge);

/// @brief 创建账户桥接实例
/// @param options_json 端点 JSON；可为 nullptr
qtrade::account::IAccountBridge* qtrade_create_account_bridge(const char* options_json);

/// @brief 销毁由 qtrade_create_account_bridge 创建的实例
void qtrade_destroy_account_bridge(qtrade::account::IAccountBridge* bridge);

/// @brief 创建账户硬风控桥接实例
/// @param options_json 端点 JSON；可为 nullptr
qtrade::account_risk::IAccountRiskBridge* qtrade_create_account_risk_bridge(const char* options_json);

/// @brief 销毁由 qtrade_create_account_risk_bridge 创建的实例
void qtrade_destroy_account_risk_bridge(qtrade::account_risk::IAccountRiskBridge* bridge);
}  // extern "C"

#endif  // QTRADE_BRIDGE_BRIDGE_PLUGIN_ABI_H_
