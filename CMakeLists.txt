cmake_minimum_required(VERSION 3.15)
project(QuantTrading VERSION 1.0.0 LANGUAGES CXX)

# 定义根目录下的release目录路径
set(ROOT_RELEASE_DIR ${CMAKE_SOURCE_DIR}/release)
# 共享库（.so）输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_RELEASE_DIR}/lib)
# 可执行文件输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_RELEASE_DIR}/bin)


# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O0")
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
