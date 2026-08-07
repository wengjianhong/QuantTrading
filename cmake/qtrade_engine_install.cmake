# ---------------------------------------------------------------------------
# Install rules for find_package(qtrade_engine CONFIG).
#
# Default prefix: /usr/local (override with -DCMAKE_INSTALL_PREFIX=).
# Installed layout:
#   lib/libqtrade_engine.a
#   include/qtrade/...（含 sdk/ 接口头）
#   config/qtrade_engine.json（引导配置样例；进程由 qtrade_client 提供）
#   lib/cmake/qtrade_engine/...
# ---------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/qtrade_engine-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(TARGETS qtrade_engine
  EXPORT qtrade_engineTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/qtrade/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade
)

# 实现头安装到 include/qtrade（公开 #include 路径）；物理源在 src/qtrade_engine/
# 不含 adapter/（厂商适配在 qtrade_client；mock 仅供本仓单测）
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/qtrade_engine/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
    PATTERN "adapter" EXCLUDE
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/
  DESTINATION config
  FILES_MATCHING PATTERN "*.json"
)

install(EXPORT qtrade_engineTargets
  FILE qtrade_engineTargets.cmake
  NAMESPACE qtrade_engine::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qtrade_engine
)

install(FILES
  ${CMAKE_CURRENT_LIST_DIR}/qtrade_engine-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/qtrade_engine-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qtrade_engine
)
