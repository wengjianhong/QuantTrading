# ---------------------------------------------------------------------------
# qtrade_engine: single static library (common + engine + test mock adapters)
# ---------------------------------------------------------------------------

file(GLOB_RECURSE QTRADE_ENGINE_COMMON_SRC CONFIGURE_DEPENDS
  ${QTRADE_ENGINE_SRC_QTRADE_DIR}/common/*/*.cpp)
file(GLOB_RECURSE QTRADE_ENGINE_ENGINE_SRC CONFIGURE_DEPENDS
  ${QTRADE_ENGINE_SRC_QTRADE_DIR}/engine/*.cpp)
file(GLOB_RECURSE QTRADE_ENGINE_MOCK_ADAPTER_SRC CONFIGURE_DEPENDS
  ${QTRADE_ENGINE_SRC_QTRADE_DIR}/adapter/*.cpp)

add_library(qtrade_engine STATIC
  ${QTRADE_ENGINE_COMMON_SRC}
  ${QTRADE_ENGINE_ENGINE_SRC}
  ${QTRADE_ENGINE_MOCK_ADAPTER_SRC}
)

target_include_directories(qtrade_engine PUBLIC
  $<BUILD_INTERFACE:${QTRADE_ENGINE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${QTRADE_ENGINE_INCLUDE_OVERLAY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(qtrade_engine PRIVATE
  ${QTRADE_ENGINE_SRC_DIR}
)
target_link_libraries(qtrade_engine PUBLIC
  Threads::Threads
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  cpputils::cpputils
  ${CMAKE_DL_LIBS}
)
