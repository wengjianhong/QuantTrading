# ---------------------------------------------------------------------------
# Install rules for find_package(qtrade CONFIG).
#
# Default prefix: /usr/local (override with -DCMAKE_INSTALL_PREFIX=).
# Installed layout:
#   lib/libqtrade_{common,core}.a
#   include/qtrade/...
#   include/qtrade_sdk/...
#   config/qtrade_engine.json（引导配置样例；进程由 qtrade_client 提供）
#   lib/cmake/qtrade/...
# ---------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/qtrade-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(TARGETS qtrade_common qtrade_core
  EXPORT qtradeTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/qtrade/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/qtrade_sdk/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade_sdk
)

# Engine / common 实现头（#include "qtrade/engine/..."、"qtrade/common/..."）
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/qtrade/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/qtrade_sdk/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade_sdk
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/
  DESTINATION config
  FILES_MATCHING PATTERN "*.json"
)

install(EXPORT qtradeTargets
  FILE qtradeTargets.cmake
  NAMESPACE qtrade::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qtrade
)

install(FILES
  ${CMAKE_CURRENT_LIST_DIR}/qtrade-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/qtrade-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qtrade
)
