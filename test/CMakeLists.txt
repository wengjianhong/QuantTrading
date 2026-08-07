
# Output Directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin)

# Dependencies
find_package(GTest 1.14.0 REQUIRED)

# Source Files
file(GLOB_RECURSE ALL_TEST_FILES CONFIGURE_DEPENDS "test_*.cpp")

# 测试用 Stub Api（仅测试链接；生产 mock 在 qtrade_client）
set(QTRADE_ENGINE_TEST_STUB_SOURCES
  stubs/stub_quote_api.cpp
  stubs/stub_quote_spi.cpp
  stubs/stub_trader_api.cpp
  stubs/stub_trader_spi.cpp
)

# Build Executable
add_executable(qtrade_tests
  ${ALL_TEST_FILES}
  ${QTRADE_ENGINE_TEST_STUB_SOURCES}
)

# Include Directories
target_include_directories(qtrade_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link Libraries
target_link_libraries(qtrade_tests PRIVATE
  qtrade_engine
  GTest::gtest
  GTest::gtest_main
)

# Discover and Run Tests
include(GoogleTest)
gtest_discover_tests(qtrade_tests
  PROPERTIES
    LABELS "unit_test"
    DISCOVERY_TIMEOUT 60
    # StrategyPluginLoader 可选：指向 qtrade_strategy 安装/构建产物目录
    ENVIRONMENT "QTRADE_STRATEGY_PLUGIN_DIR=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/strategies"
)
