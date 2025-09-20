cmake_minimum_required(VERSION 3.15)
project(quant_trading_system VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=address")

# 查找依赖
find_package(Boost 1.70 REQUIRED COMPONENTS system thread filesystem)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

# 包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/core
    ${PROJECT_SOURCE_DIR}/plugins
    ${PROJECT_SOURCE_DIR}/services
    ${PROJECT_SOURCE_DIR}/shared
    ${PROJECT_SOURCE_DIR}/api
)

# 子目录
add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(services)
add_subdirectory(api)
add_subdirectory(tests)
add_subdirectory(infrastructure)
