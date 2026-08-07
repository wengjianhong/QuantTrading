/// @file      bridge_plugin_loader.cpp
/// @brief     桥接插件加载器实现
/// @author    wengjianhong
/// @date      2026-08-06
/// @copyright CC BY-NC-SA 4.0
#include "qtrade/engine/bridge/bridge_plugin_loader.hpp"

#include <spdlog/spdlog.h>

#include <dlfcn.h>
#include <filesystem>

namespace qtrade::engine::bridge {
namespace {

void* Lookup(void* dl_handle, const char* symbol) {
  dlerror();
  void* sym = dlsym(dl_handle, symbol);
  if (dlerror() != nullptr) {
    return nullptr;
  }
  return sym;
}

template <typename Fn>
Fn LookupFn(void* dl_handle, const char* symbol) {
  return reinterpret_cast<Fn>(Lookup(dl_handle, symbol));
}

template <typename Fn>
Fn LookupFnRequired(void* dl_handle, const char* symbol) {
  dlerror();
  void* sym = dlsym(dl_handle, symbol);
  if (const char* err = dlerror(); err != nullptr) {
    spdlog::error("[BridgePluginLoader] dlsym {}: {}", symbol, err);
    return nullptr;
  }
  return reinterpret_cast<Fn>(sym);
}

}  // namespace

BridgePluginLoader::BridgePluginLoader() = default;

BridgePluginLoader::~BridgePluginLoader() {
  UnloadAll();
}

ErrorCode BridgePluginLoader::LoadDirectory(const std::string& directory) {
  std::error_code error_code;
  const auto dir = std::filesystem::path(directory);
  if (!std::filesystem::is_directory(dir, error_code)) {
    spdlog::error("[BridgePluginLoader] not a directory: {} ({})", directory, error_code.message());
    return ErrorCode::kNotSuchFileOrDirectory;
  }

  std::lock_guard lock(mutex_);
  int loaded = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir, error_code)) {
    if (error_code) {
      spdlog::error("[BridgePluginLoader] iterate {}: {}", directory, error_code.message());
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

  spdlog::info("[BridgePluginLoader] loaded {} plugin(s) from {}", loaded, directory);
  return ErrorCode::kSuccess;
}

ErrorCode BridgePluginLoader::LoadFile(const std::string& so_path) {
  std::lock_guard lock(mutex_);
  return LoadFileLocked(so_path);
}

ErrorCode BridgePluginLoader::LoadFileLocked(const std::string& so_path) {
  void* dl_handle = dlopen(so_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (dl_handle == nullptr) {
    spdlog::error("[BridgePluginLoader] dlopen {}: {}", so_path, dlerror());
    return ErrorCode::kDynamicLibraryLoadError;
  }

  const auto abi_version = LookupFnRequired<BridgeAbiVersionFn>(dl_handle, QTRADE_BRIDGE_SYM_ABI_VERSION);
  const auto plugin_name_fn = LookupFnRequired<BridgePluginNameFn>(dl_handle, QTRADE_BRIDGE_SYM_PLUGIN_NAME);
  if (abi_version == nullptr || plugin_name_fn == nullptr) {
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolNotFound;
  }

  const int version = abi_version();
  if (version != QTRADE_BRIDGE_ABI_VERSION) {
    spdlog::error(
      "[BridgePluginLoader] ABI mismatch path={} got={} expect={}", so_path, version, QTRADE_BRIDGE_ABI_VERSION);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolTypeMismatch;
  }

  const char* name_cstr = plugin_name_fn();
  if (name_cstr == nullptr || name_cstr[0] == '\0') {
    spdlog::error("[BridgePluginLoader] empty plugin name: {}", so_path);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolParseError;
  }
  const std::string plugin_name = name_cstr;

  BridgePluginEntry entry;
  entry.dl_handle = dl_handle;
  entry.path = so_path;
  entry.plugin_name = plugin_name;
  entry.create_config = LookupFn<ConfigBridgeCreateFn>(dl_handle, QTRADE_BRIDGE_SYM_CREATE_CONFIG);
  entry.destroy_config = LookupFn<ConfigBridgeDestroyFn>(dl_handle, QTRADE_BRIDGE_SYM_DESTROY_CONFIG);
  entry.create_account = LookupFn<AccountBridgeCreateFn>(dl_handle, QTRADE_BRIDGE_SYM_CREATE_ACCOUNT);
  entry.destroy_account = LookupFn<AccountBridgeDestroyFn>(dl_handle, QTRADE_BRIDGE_SYM_DESTROY_ACCOUNT);
  entry.create_account_risk = LookupFn<AccountRiskBridgeCreateFn>(dl_handle, QTRADE_BRIDGE_SYM_CREATE_ACCOUNT_RISK);
  entry.destroy_account_risk =
    LookupFn<AccountRiskBridgeDestroyFn>(dl_handle, QTRADE_BRIDGE_SYM_DESTROY_ACCOUNT_RISK);

  const bool has_config = entry.create_config != nullptr && entry.destroy_config != nullptr;
  const bool has_account = entry.create_account != nullptr && entry.destroy_account != nullptr;
  const bool has_account_risk = entry.create_account_risk != nullptr && entry.destroy_account_risk != nullptr;
  if (!has_config && !has_account && !has_account_risk) {
    spdlog::error("[BridgePluginLoader] no complete create/destroy pair: {}", so_path);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolNotFound;
  }

  // 半套符号视为错误，避免 create 成功却无法 destroy
  if ((entry.create_config != nullptr) != (entry.destroy_config != nullptr) ||
      (entry.create_account != nullptr) != (entry.destroy_account != nullptr) ||
      (entry.create_account_risk != nullptr) != (entry.destroy_account_risk != nullptr)) {
    spdlog::error("[BridgePluginLoader] mismatched create/destroy pair: {}", so_path);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolNotFound;
  }

  if (plugins_.contains(plugin_name)) {
    spdlog::warn(
      "[BridgePluginLoader] duplicate plugin name={}, keep first ({})", plugin_name, plugins_.at(plugin_name).path);
    dlclose(dl_handle);
    return ErrorCode::kDynamicLibrarySymbolRegisterError;
  }

  plugins_.emplace(plugin_name, std::move(entry));
  spdlog::info("[BridgePluginLoader] loaded plugin={} path={} config={} account={} account_risk={}",
               plugin_name,
               so_path,
               has_config,
               has_account,
               has_account_risk);
  return ErrorCode::kSuccess;
}

bool BridgePluginLoader::HasPlugin(const std::string& plugin_name) const {
  std::lock_guard lock(mutex_);
  return FindLocked(plugin_name) != nullptr;
}

std::vector<std::string> BridgePluginLoader::ListPlugins() const {
  std::lock_guard lock(mutex_);
  std::vector<std::string> names;
  names.reserve(plugins_.size());
  for (const auto& [name, _] : plugins_) {
    names.push_back(name);
  }
  return names;
}

const BridgePluginEntry* BridgePluginLoader::FindLocked(const std::string& plugin_name) const {
  const auto it = plugins_.find(plugin_name);
  return it == plugins_.end() ? nullptr : &it->second;
}

ConfigBridgePtr BridgePluginLoader::CreateConfigBridge(const std::string& plugin_name,
                                                       const char* options_json) const {
  std::lock_guard lock(mutex_);
  const BridgePluginEntry* entry = FindLocked(plugin_name);
  if (entry == nullptr || entry->create_config == nullptr || entry->destroy_config == nullptr) {
    return {nullptr, [](qtrade::config::IConfigBridge*) {}};
  }
  auto* raw = entry->create_config(options_json);
  if (raw == nullptr) {
    return {nullptr, [](qtrade::config::IConfigBridge*) {}};
  }
  return ConfigBridgePtr{raw, entry->destroy_config};
}

AccountBridgePtr BridgePluginLoader::CreateAccountBridge(const std::string& plugin_name,
                                                         const char* options_json) const {
  std::lock_guard lock(mutex_);
  const BridgePluginEntry* entry = FindLocked(plugin_name);
  if (entry == nullptr || entry->create_account == nullptr || entry->destroy_account == nullptr) {
    return {nullptr, [](qtrade::account::IAccountBridge*) {}};
  }
  auto* raw = entry->create_account(options_json);
  if (raw == nullptr) {
    return {nullptr, [](qtrade::account::IAccountBridge*) {}};
  }
  return AccountBridgePtr{raw, entry->destroy_account};
}

AccountRiskBridgePtr BridgePluginLoader::CreateAccountRiskBridge(const std::string& plugin_name,
                                                                 const char* options_json) const {
  std::lock_guard lock(mutex_);
  const BridgePluginEntry* entry = FindLocked(plugin_name);
  if (entry == nullptr || entry->create_account_risk == nullptr || entry->destroy_account_risk == nullptr) {
    return {nullptr, [](qtrade::account_risk::IAccountRiskBridge*) {}};
  }
  auto* raw = entry->create_account_risk(options_json);
  if (raw == nullptr) {
    return {nullptr, [](qtrade::account_risk::IAccountRiskBridge*) {}};
  }
  return AccountRiskBridgePtr{raw, entry->destroy_account_risk};
}

void BridgePluginLoader::UnloadAll() {
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

}  // namespace qtrade::engine::bridge
