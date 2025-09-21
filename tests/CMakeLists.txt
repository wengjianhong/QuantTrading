# 最低CMake版本要求
cmake_minimum_required(VERSION 3.10)

# 测试目标名称
project(qt_gtest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项：启用警告、调试信息，优化测试性能
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -g -O2")
# 多线程支持
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# 查找GTest库
find_package(GTest REQUIRED)
if(GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "GTest library not found!")
endif()

# 收集测试源文件
set(TEST_SOURCES
    base/safe_queue/test_safe_queue.cpp
    base/thread_pool/test_thread_pool.cpp
)

# 添加测试可执行文件
add_executable(${PROJECT_NAME} ${TEST_SOURCES})

# 包含项目根目录（确保测试能引用到base/下的头文件）
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/..  # 项目根目录
)

# 为当前目标（${PROJECT_NAME}）设置库搜索目录
target_link_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../release/lib  # qtbase 库所在目录
)

# 链接依赖库：GTest、无锁队列和线程池的实现库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        GTest::GTest
        GTest::Main
        qtbase  # 依赖前面实现的基础库（无锁队列+线程池）
        atomic
)

# 自动发现测试用例并添加到CTest
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    PROPERTIES
        LABELS "unit"
        DISCOVERY_TIMEOUT  # 超时时间（秒）
        60
)

# 安装测试目标（可选）
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/tests
)
