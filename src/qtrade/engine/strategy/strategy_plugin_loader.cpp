/// @file      strategy_plugin_loader.cpp
/// @brief     策略插件加载器实现
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/strategy/strategy_plugin_loader.hpp"

#include "qtrade/error_code/error_codes.hpp"

#include <spdlog/spdlog.h>

#include <dlfcn.h>
#include <filesystem>

namespace qtrade::engine::strategy {
namespace {

void* Lookup(void* dl_handle, const char* symbol) {
  dlerror();
  void* sym = dlsym(dl_handle, symbol);
  if (const char* err = dlerror(); err != nullptr) {
    spdlog::error("[StrategyPluginLoader] dlsym {}: {}", symbol, err);
    return nullptr;
  }
  return sym;
}

template <typename Fn>
Fn LookupFn(void* dl_handle, const char* symbol) {
  return reinterpret_cast<Fn>(Lookup(dl_handle, symbol));
}

}  // namespace

StrategyPluginLoader::StrategyPluginLoader() = default;

StrategyPluginLoader::~StrategyPluginLoader() {
  UnloadAll();
}

ErrorCode StrategyPluginLoader::LoadStrategyPlugin(const std::string& directory) {
  std::error_code error_code;
  const auto dir = std::filesystem::path(directory);
  if (!std::filesystem::is_directory(dir, error_code)) {
    spdlog::error("[StrategyPluginLoader] not a directory: {} ({})", directory, error_code.message());
    return ErrorCode::kNotSuchFileOrDirectory;
  }

  std::lock_guard lock(mutex_);
  int loaded = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir, error_code)) {
    if (error_code) {
      spdlog::error("[StrategyPluginLoader] iterate {}: {}", directory, error_code.message());
      return ErrorCode::kDynamicLibraryLoadError;
    }
    if (!entry.is_regular_file()) {
      continue;
    }
    if (entry.path().extension() != ".so") {
      continue;
    }
    if (LoadFileLocked(entry.path().string()) == ErrorCode::kSuccess) {
      ++loaded;
    }
  }

  spdlog::info("[StrategyPluginLoader] loaded {} plugin(s) from {}", loaded, directory);
  return ErrorCode::kSuccess;
}

ErrorCode StrategyPluginLoader::LoadFile(const std::string& so_path) {
  std::lock_guard lock(mutex_);
  return LoadFileLocked(so_path);
}

ErrorCode StrategyPluginLoader::LoadFileLocked(const std::string& so_path) {
  void* dl_handle = dlopen(so_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (dl_handle == nullptr) {
    spdlog::error("[StrategyPluginLoader] dlopen {}: {}", so_path, dlerror());
    return ErrorCode::kDynamicLibraryLoadError;
  }

  const auto create = LookupFn<StrategyCreateFn>(dl_handle, QTRADE_STRATEGY_SYM_CREATE);
  const auto destroy = LookupFn<StrategyDestroyFn>(dl_handle, QTRADE_STRATEGY_SYM_DESTROY);
  const auto abi_version = LookupFn<StrategyAbiVersionFn>(dl_handle, QTRADE_STRATEGY_SYM_ABI_VERSION);
  const auto plugin_name_fn = LookupFn<StrategyPluginNameFn>(dl_handle, QTRADE_STRATEGY_SYM_PLUGIN_NAME);

  if (abi_version == nullptr || plugin_name_fn == nullptr || create == nullptr || destroy == nullptr) {
    dlclose(dl_handle);
    spdlog::error("[StrategyPluginLoader] symbol not found path={}", so_path);
    return ErrorCode::kDynamicLibrarySymbolNotFound;
  }

  const int version = abi_version();
  if (version != QTRADE_STRATEGY_ABI_VERSION) {
    spdlog::error(
      "[StrategyPluginLoader] ABI mismatch path={} got={} expect={}", so_path, version, QTRADE_STRATEGY_ABI_VERSION);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolTypeMismatch;
  }

  const char* name_cstr = plugin_name_fn();
  if (name_cstr == nullptr || name_cstr[0] == '\0') {
    spdlog::error("[StrategyPluginLoader] empty plugin name: {}", so_path);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolParseError;
  }
  const std::string plugin_name = name_cstr;

  if (plugins_.contains(plugin_name)) {
    spdlog::warn(
      "[StrategyPluginLoader] duplicate plugin name={}, keep first ({})", plugin_name, plugins_.at(plugin_name).path);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolRegisterError;
  }

  StrategyPluginEntry entry;
  entry.dl_handle = dl_handle;
  entry.path = so_path;
  entry.plugin_name = plugin_name;
  entry.create = create;
  entry.destroy = destroy;
  plugins_.emplace(plugin_name, std::move(entry));

  spdlog::info("[StrategyPluginLoader] loaded plugin={} path={}", plugin_name, so_path);
  return ErrorCode::kSuccess;
}

bool StrategyPluginLoader::HasPlugin(const std::string& plugin_name) const {
  std::lock_guard lock(mutex_);
  return FindLocked(plugin_name) != nullptr;
}

std::vector<std::string> StrategyPluginLoader::ListPlugins() const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> names;
  names.reserve(plugins_.size());
  for (const auto& [name, _] : plugins_) {
    names.push_back(name);
  }
  return names;
}

const StrategyPluginEntry* StrategyPluginLoader::FindLocked(const std::string& plugin_name) const {
  const auto it = plugins_.find(plugin_name);
  return it == plugins_.end() ? nullptr : &it->second;
}

StrategyPtr StrategyPluginLoader::Create(const std::string& plugin_name) const {
  std::lock_guard lock(mutex_);
  const StrategyPluginEntry* entry = FindLocked(plugin_name);
  if (entry == nullptr || entry->create == nullptr || entry->destroy == nullptr) {
    return {nullptr, [](IStrategy*) {}};
  }
  IStrategy* raw = entry->create();
  if (raw == nullptr) {
    return {nullptr, [](IStrategy*) {}};
  }
  return StrategyPtr{raw, entry->destroy};
}

void StrategyPluginLoader::UnloadAll() {
  std::lock_guard lock(mutex_);
  for (auto& [name, entry] : plugins_) {
    (void)name;
    if (entry.dl_handle != nullptr) {
      dlclose(entry.dl_handle);
      entry.dl_handle = nullptr;
    }
  }
  plugins_.clear();
}

}  // namespace qtrade::engine::strategy
