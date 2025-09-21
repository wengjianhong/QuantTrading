# base/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(qtbase LANGUAGES CXX)


# 定义根目录下的release目录路径
set(ROOT_RELEASE_DIR ${CMAKE_SOURCE_DIR}/../release)
# 共享库（.so）输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_RELEASE_DIR}/lib)
# 可执行文件输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_RELEASE_DIR}/bin)

# 按功能目录自动获取源文件（清晰且减少手动操作）
file(GLOB SAFE_QUEUE_SOURCES "common/safe_queue/*")
file(GLOB THREAD_POOL_SOURCES "common/thread_pool/*")
file(GLOB DATA_TYPES_SOURCES "data_types/*")

# 合并源文件（便于后续维护，新增目录只需添加一行 GLOB）
set(SOURCES
    ${SAFE_QUEUE_SOURCES}
    ${THREAD_POOL_SOURCES}
    ${DATA_TYPES_SOURCES}
)

# 定义共享库目标
add_library(qtbase SHARED ${SOURCES})


# 设置库版本（可选，便于版本管理）
set_target_properties(qtbase PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# 配置编译选项（与测试模块保持一致）
target_compile_features(qtbase PUBLIC cxx_std_17)
target_compile_options(qtbase PRIVATE -Wall -Wextra -Werror -O2)

# 暴露头文件路径（让依赖该库的模块能找到头文件）
target_include_directories(qtbase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}  # 包含base/目录，外部可直接#include "common/..."
)

# 链接依赖（如线程库）
target_link_libraries(qtbase PRIVATE pthread)
