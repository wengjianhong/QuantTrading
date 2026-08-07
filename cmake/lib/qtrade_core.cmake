# ---------------------------------------------------------------------------
# qtrade_core: trading engine (qtrade_sdk + engine); no gRPC clients
# ---------------------------------------------------------------------------

file(GLOB_RECURSE ADAPTER_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_SDK_DIR}/*.cpp)
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_DIR}/engine/*.cpp)

add_library(qtrade_core STATIC
  ${ADAPTER_SRC}
  ${ENGINE_SRC}
)

target_include_directories(qtrade_core PUBLIC
  $<BUILD_INTERFACE:${QTRADE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${QTRADE_SRC_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(qtrade_core PRIVATE
  ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_core PUBLIC
  qtrade_common
  nlohmann_json::nlohmann_json
  ${CMAKE_DL_LIBS}
)
