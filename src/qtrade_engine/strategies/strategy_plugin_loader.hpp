/// @file      strategy_plugin_loader.hpp
/// @brief     策略 .so 插件加载器
/// @details   扫描目录 dlopen，按 C ABI 注册插件；Create 返回带自定义删除器的 unique_ptr。
///            句柄在 Unload/析构前保持打开，避免策略实例仍存活时 dlclose。
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#ifndef QTRADE_ENGINE_STRATEGY_PLUGIN_LOADER_HPP_
#define QTRADE_ENGINE_STRATEGY_PLUGIN_LOADER_HPP_

#include <qtrade/error_code/error_codes.hpp>
#include <qtrade/strategy/strategy.hpp>
#include <qtrade/strategy/strategy_plugin_abi.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace qtrade::engine::strategies {
using qtrade::strategy::IStrategy;

/// @brief 与 ABI 导出符号签名一致的函数指针（供 dlsym 强转）
using StrategyCreateFn = decltype(&::qtrade_strategy_create);
using StrategyDestroyFn = decltype(&::qtrade_strategy_destroy);
using StrategyPluginNameFn = decltype(&::qtrade_strategy_plugin_name);
using StrategyAbiVersionFn = decltype(&::qtrade_strategy_abi_version);

/// @brief 策略实例指针
using StrategyPtr = std::unique_ptr<IStrategy, StrategyDestroyFn>;

/// @brief 策略插件条目
struct StrategyPluginEntry {
  /// 动态库句柄
  void* dl_handle = nullptr;
  /// 动态库路径
  std::string path;
  /// 策略插件名称
  std::string plugin_name;
  /// 创建函数
  StrategyCreateFn create = nullptr;
  /// 销毁函数
  StrategyDestroyFn destroy = nullptr;
};

/// @brief 策略动态库加载与按 plugin 名创建
class StrategyPluginLoader {
 public:
  StrategyPluginLoader();
  ~StrategyPluginLoader();

  StrategyPluginLoader(const StrategyPluginLoader&) = delete;
  StrategyPluginLoader& operator=(const StrategyPluginLoader&) = delete;

  /// @brief 扫描目录下所有 .so 并加载合法插件（可多次调用追加）
  /// @param directory 策略插件目录绝对或相对路径
  /// @return 成功返回 kSuccess
  ErrorCode LoadStrategyPlugin(const std::string& directory);

  /// @brief 加载单个 .so
  /// @param so_path 策略插件路径
  /// @return 成功返回 kSuccess
  ErrorCode LoadFile(const std::string& so_path);

  /// @brief 是否已加载指定策略插件
  /// @param plugin_name 策略插件名称，如: libmy_strategy.so 的 my_strategy
  /// @return 是否已加载
  [[nodiscard]] bool HasPlugin(const std::string& plugin_name) const;

  /// @brief 获取策略插件名称列表
  /// @return 策略插件名称列表
  [[nodiscard]] std::vector<std::string> ListPlugins() const;

  /// @brief 按 ABI 插件名创建策略实例
  /// @param plugin_name 策略插件名称，如: libmy_strategy.so 的 my_strategy
  /// @return 策略实例；失败返回空 StrategyPtr
  [[nodiscard]] StrategyPtr Create(const std::string& plugin_name) const;

  /// @brief 关闭全部句柄
  /// @warning 调用前须确保策略实例已销毁
  void UnloadAll();

 private:
  /// 互斥锁
  mutable std::mutex mutex_;

  /// 策略插件名称 → 插件条目
  std::unordered_map<std::string, StrategyPluginEntry> plugins_;

  /// @brief 加载策略插件（内部锁保护）
  /// @param so_path 策略插件路径
  /// @return 成功返回 kSuccess
  ErrorCode LoadFileLocked(const std::string& so_path);

  /// @brief 查找策略插件（内部锁保护）
  /// @param plugin_name 策略插件名称
  /// @return 策略插件条目；未找到返回 nullptr
  [[nodiscard]] const StrategyPluginEntry* FindLocked(const std::string& plugin_name) const;
};

}  // namespace qtrade::engine::strategies

#endif  // QTRADE_ENGINE_STRATEGY_PLUGIN_LOADER_HPP_
