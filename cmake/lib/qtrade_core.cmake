# ---------------------------------------------------------------------------
# qtrade_core: trading engine (qtrade_sdk mock/emt, engine, client)
# ---------------------------------------------------------------------------

# Source Files
file(GLOB_RECURSE ADAPTER_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_SDK_DIR}/*.cpp)
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_DIR}/engine/*.cpp)
file(GLOB_RECURSE CLIENT_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_DIR}/client/*.cpp)
list(APPEND CORE_SRC_FILES
    ${ADAPTER_SRC}
    ${ENGINE_SRC}
    ${CLIENT_SRC}
)

# Build Core Library
add_library(qtrade_core STATIC ${CORE_SRC_FILES})

## Include Public Directories
target_include_directories(qtrade_core PUBLIC
    $<BUILD_INTERFACE:${QTRADE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
## Include Private Directories
target_include_directories(qtrade_core PRIVATE
    ${QTRADE_SRC_DIR}
)
## Link Public Libraries
target_link_libraries(qtrade_core PUBLIC
    qtrade_common
    qtrade_proto
    nlohmann_json::nlohmann_json
    ${CMAKE_DL_LIBS}
)
