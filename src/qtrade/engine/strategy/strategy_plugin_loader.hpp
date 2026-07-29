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

namespace qtrade::engine::strategy {

/// @brief 策略动态库加载与按 plugin 名创建
class StrategyPluginLoader {
 public:
  StrategyPluginLoader();
  ~StrategyPluginLoader();

  StrategyPluginLoader(const StrategyPluginLoader&) = delete;
  StrategyPluginLoader& operator=(const StrategyPluginLoader&) = delete;

  /// @brief 扫描目录下所有 .so 并加载合法插件（可多次调用追加）
  /// @param directory 插件目录绝对或相对路径
  /// @return 至少成功加载一个插件返回 kSuccess；目录无效返回错误；全部失败返回 kNotFound
  ErrorCode LoadDirectory(const std::string& directory);

  /// @brief 加载单个 .so
  /// @param so_path 动态库路径
  /// @return 成功返回 kSuccess
  ErrorCode LoadFile(const std::string& so_path);

  /// @brief 是否已加载指定 plugin（支持 ABI 名、文件名、去 lib/.so 的 stem）
  [[nodiscard]] bool HasPlugin(const std::string& plugin) const;

  /// @brief 已加载的插件规范名列表（ABI plugin_name）
  [[nodiscard]] std::vector<std::string> ListPlugins() const;

  /// @brief 按 plugin 名创建策略实例
  /// @param plugin 配置中的 plugin 字段
  /// @return 实例；失败返回 nullptr
  [[nodiscard]] std::unique_ptr<qtrade::strategy::IStrategy, void (*)(qtrade::strategy::IStrategy*)> Create(
    const std::string& plugin) const;

  /// @brief 关闭全部句柄（调用前须确保策略实例已销毁）
  void UnloadAll();

 private:
  struct PluginEntry {
    void* handle = nullptr;
    std::string path;
    std::string plugin_name;
    int (*abi_version)() = nullptr;
    const char* (*plugin_name_fn)() = nullptr;
    qtrade::strategy::IStrategy* (*create)() = nullptr;
    void (*destroy)(qtrade::strategy::IStrategy*) = nullptr;
  };

  mutable std::mutex mutex_;
  /// 规范 plugin_name → 条目
  std::unordered_map<std::string, PluginEntry> plugins_;
  /// 别名（文件名 / stem）→ 规范 plugin_name
  std::unordered_map<std::string, std::string> aliases_;

  ErrorCode LoadFileLocked(const std::string& so_path);
  [[nodiscard]] const PluginEntry* FindLocked(const std::string& plugin) const;
  static void RegisterAliases(std::unordered_map<std::string, std::string>& aliases,
                              const std::string& canonical,
                              const std::string& so_path);
};

}  // namespace qtrade::engine::strategy

#endif  // QTRADE_ENGINE_STRATEGY_PLUGIN_LOADER_HPP_
