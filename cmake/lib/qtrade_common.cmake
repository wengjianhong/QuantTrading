# ---------------------------------------------------------------------------
# qtrade_common: product common (app/logging), framework common (dao/database/grpc), error codes
# ---------------------------------------------------------------------------

file(GLOB_RECURSE QTRADE_PRODUCT_COMMON_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_DIR}/common/*/*.cpp)
file(GLOB_RECURSE QTRADE_FRAMEWORK_COMMON_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_FRAMEWORK_DIR}/common/*/*.cpp)
file(GLOB_RECURSE DAO_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_FRAMEWORK_DIR}/dao/*.cpp)
file(GLOB_RECURSE PUBLIC_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_DIR}/error_code/*.cpp)
file(GLOB_RECURSE COMMON_GRPC_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_FRAMEWORK_DIR}/common/grpc/*.cpp)

# Build Common Library
add_library(qtrade_common STATIC
    ${QTRADE_PRODUCT_COMMON_SRC}
    ${QTRADE_FRAMEWORK_COMMON_SRC}
    ${DAO_SRC}
    ${PUBLIC_SRC}
    ${COMMON_GRPC_SRC}
)
## Include Public Directories
target_include_directories(qtrade_common PUBLIC
    $<BUILD_INTERFACE:${QTRADE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
## Include Private Directories
target_include_directories(qtrade_common PRIVATE
    ${QTRADE_SRC_DIR}
)
## Link Public Libraries
target_link_libraries(qtrade_common PUBLIC
    Threads::Threads
    spdlog::spdlog
    qtrade_proto
    nlohmann_json::nlohmann_json
    cpputils::cpputils
)
