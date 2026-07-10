# ---------------------------------------------------------------------------
# qtrade_common: logging, process bootstrap, error codes, gRPC infrastructure
# ---------------------------------------------------------------------------

file(GLOB_RECURSE COMMON_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_DIR}/common/*/*.cpp)
file(GLOB_RECURSE DAO_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_DIR}/dao/*.cpp)
file(GLOB_RECURSE PUBLIC_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_DIR}/public/*.cpp)
file(GLOB_RECURSE COMMON_GRPC_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_DIR}/common/grpc/*.cpp)

# Build Common Library
add_library(qtrade_common STATIC ${COMMON_SRC} ${DAO_SRC} ${PUBLIC_SRC} ${COMMON_GRPC_SRC})
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
