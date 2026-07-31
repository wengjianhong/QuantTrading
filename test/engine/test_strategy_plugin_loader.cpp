#include "qtrade/engine/strategy/strategy_plugin_loader.hpp"
#include "qtrade/strategy/strategy_plugin_abi.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <string>

namespace {

std::string PluginDir() {
  if (const char* from_env = std::getenv("QTRADE_STRATEGY_PLUGIN_DIR"); from_env != nullptr && from_env[0] != '\0') {
    return from_env;
  }
  // 默认指向本机构建产物：<build>/lib/strategies
  const auto candidates = {
    std::filesystem::path("lib/strategies"),
    std::filesystem::path("../lib/strategies"),
    std::filesystem::path("../../lib/strategies"),
  };
  for (const auto& candidate : candidates) {
    if (std::filesystem::is_directory(candidate)) {
      return std::filesystem::absolute(candidate).string();
    }
  }
  return {};
}

}  // namespace

TEST(StrategyPluginLoader, LoadsExamplePluginAndCreatesInstance) {
  const auto dir = PluginDir();
  if (dir.empty()) {
    GTEST_SKIP() << "strategy plugin directory not found; set QTRADE_STRATEGY_PLUGIN_DIR";
  }

  qtrade::engine::strategy::StrategyPluginLoader loader;
  ASSERT_EQ(loader.LoadStrategyPlugin(dir), qtrade::ErrorCode::kSuccess);
  EXPECT_TRUE(loader.HasPlugin("example"));
  EXPECT_FALSE(loader.HasPlugin("example_strategy"));
  EXPECT_FALSE(loader.HasPlugin("libexample_strategy.so"));

  auto strategy = loader.Create("example");
  ASSERT_NE(strategy, nullptr);
  qtrade::strategy::StrategyConfig config;
  config.strategy_id = "ut-example";
  config.strategy_name = "example";
  config.enabled = true;
  EXPECT_EQ(strategy->Init(config), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(strategy->GetStrategyConfig().strategy_id, "ut-example");
  EXPECT_EQ(strategy->Start(), qtrade::ErrorCode::kSuccess);
  strategy->Stop();
}
