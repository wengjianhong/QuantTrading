
# Output Directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin)

# Dependencies
find_package(GTest 1.14.0 REQUIRED)

# Source Files
file(GLOB_RECURSE ALL_TEST_FILES CONFIGURE_DEPENDS "test_*.cpp")

# Build Executable
add_executable(qtrade_tests ${ALL_TEST_FILES})

# Include Directories
target_include_directories(qtrade_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link Libraries
target_link_libraries(qtrade_tests PRIVATE
    qtrade_account_risk_service_static
    qtrade_core
    GTest::gtest
    GTest::gtest_main
)

# Ensure example_strategy is built before running StrategyPluginLoader tests
add_dependencies(qtrade_tests example_strategy)

# Discover and Run Tests
include(GoogleTest)
gtest_discover_tests(qtrade_tests
    PROPERTIES
        LABELS "unit_test"
        DISCOVERY_TIMEOUT 60
        # StrategyPluginLoader tests locate .so via this environment variable
        ENVIRONMENT "QTRADE_STRATEGY_PLUGIN_DIR=${CMAKE_BINARY_DIR}/lib/strategies"
)
