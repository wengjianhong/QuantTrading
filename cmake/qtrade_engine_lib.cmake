# ---------------------------------------------------------------------------
# qtrade_engine: shared library (common + engine modules)
# ---------------------------------------------------------------------------

file(GLOB_RECURSE QTRADE_ENGINE_ALL_SRC CONFIGURE_DEPENDS
  ${QTRADE_ENGINE_SRC_QTRADE_DIR}/*.cpp)

set(QTRADE_ENGINE_COMMON_SRC ${QTRADE_ENGINE_ALL_SRC})
list(FILTER QTRADE_ENGINE_COMMON_SRC INCLUDE REGEX "/common/")

set(QTRADE_ENGINE_ENGINE_SRC ${QTRADE_ENGINE_ALL_SRC})
list(FILTER QTRADE_ENGINE_ENGINE_SRC EXCLUDE REGEX "/common/")

add_library(qtrade_engine SHARED
  ${QTRADE_ENGINE_COMMON_SRC}
  ${QTRADE_ENGINE_ENGINE_SRC}
)

set_target_properties(qtrade_engine PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  OUTPUT_NAME qtrade_engine
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
