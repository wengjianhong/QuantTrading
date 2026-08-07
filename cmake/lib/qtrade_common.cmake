# ---------------------------------------------------------------------------
# qtrade_common: product common (engine-side; no dao / service framework / proto)
# ---------------------------------------------------------------------------

file(GLOB_RECURSE QTRADE_PRODUCT_COMMON_SRC CONFIGURE_DEPENDS ${QTRADE_SRC_QTRADE_DIR}/common/*/*.cpp)

add_library(qtrade_common STATIC
  ${QTRADE_PRODUCT_COMMON_SRC}
)

target_include_directories(qtrade_common PUBLIC
  $<BUILD_INTERFACE:${QTRADE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${QTRADE_SRC_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(qtrade_common PRIVATE
  ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_common PUBLIC
  Threads::Threads
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  cpputils::cpputils
)
