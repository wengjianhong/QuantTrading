/// @file      bridge_plugin_loader.hpp
/// @brief     桥接 .so 插件加载器
/// @details   扫描目录或单文件 dlopen，按 C ABI 注册；Create* 返回带自定义删除器的 unique_ptr。
///            同一插件可导出多套 create/destroy；句柄在 Unload/析构前保持打开。
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_BRIDGE_PLUGIN_LOADER_HPP_
#define QTRADE_ENGINE_BRIDGE_PLUGIN_LOADER_HPP_

#include <qtrade/bridge/account_bridge.hpp>
#include <qtrade/bridge/account_risk_bridge.hpp>
#include <qtrade/bridge/bridge_plugin_abi.h>
#include <qtrade/bridge/config_bridge.hpp>
#include <qtrade/error_code/error_codes.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::bridge {

using ConfigBridgeCreateFn = decltype(&::qtrade_create_config_bridge);
using ConfigBridgeDestroyFn = decltype(&::qtrade_destroy_config_bridge);
using AccountBridgeCreateFn = decltype(&::qtrade_create_account_bridge);
using AccountBridgeDestroyFn = decltype(&::qtrade_destroy_account_bridge);
using AccountRiskBridgeCreateFn = decltype(&::qtrade_create_account_risk_bridge);
using AccountRiskBridgeDestroyFn = decltype(&::qtrade_destroy_account_risk_bridge);
using BridgePluginNameFn = decltype(&::qtrade_bridge_plugin_name);
using BridgeAbiVersionFn = decltype(&::qtrade_bridge_abi_version);

using ConfigBridgePtr = std::unique_ptr<qtrade::config::IConfigBridge, ConfigBridgeDestroyFn>;
using AccountBridgePtr = std::unique_ptr<qtrade::account::IAccountBridge, AccountBridgeDestroyFn>;
using AccountRiskBridgePtr =
  std::unique_ptr<qtrade::account_risk::IAccountRiskBridge, AccountRiskBridgeDestroyFn>;

/// @brief 桥接插件条目
struct BridgePluginEntry {
  void* dl_handle = nullptr;
  std::string path;
  std::string plugin_name;

  ConfigBridgeCreateFn create_config = nullptr;
  ConfigBridgeDestroyFn destroy_config = nullptr;
  AccountBridgeCreateFn create_account = nullptr;
  AccountBridgeDestroyFn destroy_account = nullptr;
  AccountRiskBridgeCreateFn create_account_risk = nullptr;
  AccountRiskBridgeDestroyFn destroy_account_risk = nullptr;
};

/// @brief 桥接动态库加载与按 plugin 名创建
class BridgePluginLoader {
 public:
  BridgePluginLoader();
  ~BridgePluginLoader();

  BridgePluginLoader(const BridgePluginLoader&) = delete;
  BridgePluginLoader& operator=(const BridgePluginLoader&) = delete;

  /// @brief 扫描目录下所有 .so 并加载合法插件（可多次调用追加）
  ErrorCode LoadDirectory(const std::string& directory);

  /// @brief 加载单个 .so
  ErrorCode LoadFile(const std::string& so_path);

  [[nodiscard]] bool HasPlugin(const std::string& plugin_name) const;
  [[nodiscard]] std::vector<std::string> ListPlugins() const;

  /// @brief 按插件名创建配置桥接；options_json 传给 create（可为 nullptr）
  [[nodiscard]] ConfigBridgePtr CreateConfigBridge(const std::string& plugin_name,
                                                   const char* options_json = nullptr) const;

  [[nodiscard]] AccountBridgePtr CreateAccountBridge(const std::string& plugin_name,
                                                     const char* options_json = nullptr) const;

  [[nodiscard]] AccountRiskBridgePtr CreateAccountRiskBridge(const std::string& plugin_name,
                                                             const char* options_json = nullptr) const;

  /// @brief 关闭全部句柄；调用前须确保桥接实例已销毁
  void UnloadAll();

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, BridgePluginEntry> plugins_;

  ErrorCode LoadFileLocked(const std::string& so_path);
  [[nodiscard]] const BridgePluginEntry* FindLocked(const std::string& plugin_name) const;
};

}  // namespace qtrade::engine::bridge

#endif  // QTRADE_ENGINE_BRIDGE_PLUGIN_LOADER_HPP_
