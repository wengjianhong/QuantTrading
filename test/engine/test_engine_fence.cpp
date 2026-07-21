#include "qtrade/engine/core/engine_fence.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>

TEST(EngineFence, EnforcesSingleWriterAndMonotonicEpoch) {
  const std::string path =
    (std::filesystem::temp_directory_path() /
     ("qtrade-engine-fence-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
      .string();

  qtrade::engine::EngineFence first;
  qtrade::engine::EngineFence second;
  ASSERT_EQ(first.Acquire(path, 5), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(first.Epoch(), 5U);
  EXPECT_EQ(second.Acquire(path, 5), qtrade::ErrorCode::kResourceExhausted);

  first.Release();
  ASSERT_EQ(second.Acquire(path, 5), qtrade::ErrorCode::kSuccess);
  EXPECT_EQ(second.Epoch(), 6U);
  second.Release();
  std::filesystem::remove(path);
}
