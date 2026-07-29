/// @file      strategy_plugin_loader.cpp
/// @brief     策略插件加载器实现
/// @author    wengjianhong
/// @date      2026-07-29
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/strategy/strategy_plugin_loader.hpp"

#include <spdlog/spdlog.h>

#include <dlfcn.h>

#include <filesystem>
#include <string_view>

namespace qtrade::engine::strategy {
namespace {

void* Lookup(void* handle, const char* symbol) {
  dlerror();
  void* sym = dlsym(handle, symbol);
  if (const char* err = dlerror(); err != nullptr) {
    spdlog::error("[StrategyPluginLoader] dlsym {}: {}", symbol, err);
    return nullptr;
  }
  return sym;
}

std::string FileName(const std::string& path) {
  return std::filesystem::path(path).filename().string();
}

std::string StemName(const std::string& path) {
  auto stem = std::filesystem::path(path).stem().string();
  constexpr std::string_view kLibPrefix = "lib";
  if (stem.size() > kLibPrefix.size() && stem.compare(0, kLibPrefix.size(), kLibPrefix) == 0) {
    stem.erase(0, kLibPrefix.size());
  }
  return stem;
}

}  // namespace

StrategyPluginLoader::StrategyPluginLoader() = default;

StrategyPluginLoader::~StrategyPluginLoader() {
  UnloadAll();
}

void StrategyPluginLoader::RegisterAliases(std::unordered_map<std::string, std::string>& aliases,
                                           const std::string& canonical,
                                           const std::string& so_path) {
  aliases[canonical] = canonical;
  const auto file_name = FileName(so_path);
  const auto stem = StemName(so_path);
  if (!file_name.empty()) {
    aliases[file_name] = canonical;
  }
  if (!stem.empty()) {
    aliases[stem] = canonical;
  }
}

ErrorCode StrategyPluginLoader::LoadDirectory(const std::string& directory) {
  std::error_code ec;
  const auto dir = std::filesystem::path(directory);
  if (!std::filesystem::is_directory(dir, ec)) {
    spdlog::error("[StrategyPluginLoader] not a directory: {} ({})", directory, ec.message());
    return ErrorCode::kInvalidArgument;
  }

  std::lock_guard lock(mutex_);
  int loaded = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
    if (ec) {
      spdlog::error("[StrategyPluginLoader] iterate {}: {}", directory, ec.message());
      return ErrorCode::kSystemError;
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
  if (loaded == 0) {
    spdlog::warn("[StrategyPluginLoader] no plugin loaded from {}", directory);
    return ErrorCode::kNotFound;
  }
  spdlog::info("[StrategyPluginLoader] loaded {} plugin(s) from {}", loaded, directory);
  return ErrorCode::kSuccess;
}

ErrorCode StrategyPluginLoader::LoadFile(const std::string& so_path) {
  std::lock_guard lock(mutex_);
  return LoadFileLocked(so_path);
}

ErrorCode StrategyPluginLoader::LoadFileLocked(const std::string& so_path) {
  void* handle = dlopen(so_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (handle == nullptr) {
    spdlog::error("[StrategyPluginLoader] dlopen {}: {}", so_path, dlerror());
    return ErrorCode::kSystemError;
  }

  auto* abi_version = reinterpret_cast<int (*)()>(Lookup(handle, QTRADE_STRATEGY_SYM_ABI_VERSION));
  auto* plugin_name_fn =
    reinterpret_cast<const char* (*)()>(Lookup(handle, QTRADE_STRATEGY_SYM_PLUGIN_NAME));
  auto* create =
    reinterpret_cast<qtrade::strategy::IStrategy* (*)()>(Lookup(handle, QTRADE_STRATEGY_SYM_CREATE));
  auto* destroy =
    reinterpret_cast<void (*)(qtrade::strategy::IStrategy*)>(Lookup(handle, QTRADE_STRATEGY_SYM_DESTROY));
  if (abi_version == nullptr || plugin_name_fn == nullptr || create == nullptr || destroy == nullptr) {
    dlclose(handle);
    return ErrorCode::kSystemError;
  }

  const int version = abi_version();
  if (version != QTRADE_STRATEGY_ABI_VERSION) {
    spdlog::error("[StrategyPluginLoader] ABI mismatch path={} got={} expect={}",
                  so_path,
                  version,
                  QTRADE_STRATEGY_ABI_VERSION);
    dlclose(handle);
    return ErrorCode::kSystemError;
  }

  const char* name_cstr = plugin_name_fn();
  if (name_cstr == nullptr || name_cstr[0] == '\0') {
    spdlog::error("[StrategyPluginLoader] empty plugin name: {}", so_path);
    dlclose(handle);
    return ErrorCode::kSystemError;
  }
  const std::string plugin_name = name_cstr;

  if (plugins_.contains(plugin_name)) {
    spdlog::warn("[StrategyPluginLoader] duplicate plugin name={}, keep first ({})",
                 plugin_name,
                 plugins_.at(plugin_name).path);
    dlclose(handle);
    return ErrorCode::kSystemError;
  }

  PluginEntry entry;
  entry.handle = handle;
  entry.path = so_path;
  entry.plugin_name = plugin_name;
  entry.abi_version = abi_version;
  entry.plugin_name_fn = plugin_name_fn;
  entry.create = create;
  entry.destroy = destroy;
  plugins_.emplace(plugin_name, entry);
  RegisterAliases(aliases_, plugin_name, so_path);

  spdlog::info("[StrategyPluginLoader] loaded plugin={} path={}", plugin_name, so_path);
  return ErrorCode::kSuccess;
}

bool StrategyPluginLoader::HasPlugin(const std::string& plugin) const {
  std::lock_guard lock(mutex_);
  return FindLocked(plugin) != nullptr;
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

const StrategyPluginLoader::PluginEntry* StrategyPluginLoader::FindLocked(const std::string& plugin) const {
  const auto alias = aliases_.find(plugin);
  if (alias == aliases_.end()) {
    return nullptr;
  }
  const auto it = plugins_.find(alias->second);
  return it == plugins_.end() ? nullptr : &it->second;
}

std::unique_ptr<qtrade::strategy::IStrategy, void (*)(qtrade::strategy::IStrategy*)>
StrategyPluginLoader::Create(const std::string& plugin) const {
  std::lock_guard lock(mutex_);
  const PluginEntry* entry = FindLocked(plugin);
  if (entry == nullptr || entry->create == nullptr || entry->destroy == nullptr) {
    return {nullptr, [](qtrade::strategy::IStrategy*) {}};
  }
  qtrade::strategy::IStrategy* raw = entry->create();
  if (raw == nullptr) {
    return {nullptr, [](qtrade::strategy::IStrategy*) {}};
  }
  return {raw, entry->destroy};
}

void StrategyPluginLoader::UnloadAll() {
  std::lock_guard lock(mutex_);
  for (auto& [name, entry] : plugins_) {
    (void)name;
    if (entry.handle != nullptr) {
      dlclose(entry.handle);
      entry.handle = nullptr;
    }
  }
  plugins_.clear();
  aliases_.clear();
}

}  // namespace qtrade::engine::strategy
