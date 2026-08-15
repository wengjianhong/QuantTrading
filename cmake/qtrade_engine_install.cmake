# ---------------------------------------------------------------------------
# Install rules for find_package(qtrade_engine CONFIG).
#
# Default prefix: /usr/local (override with -DCMAKE_INSTALL_PREFIX=).
# Installed layout:
#   lib/libqtrade_engine.so
#   include/qtrade/...（仅稳定公开契约）
#   config/qtrade_engine.json
#   lib/cmake/qtrade_engine/...
#
# 实现头永不安装；common、错误码与 Result 由 qtrade_common 单独发布。
# ---------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/qtrade_engine-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(TARGETS qtrade_engine
  EXPORT qtrade_engineTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/qtrade/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade
  PATTERN "error_code" EXCLUDE
  PATTERN "structs" EXCLUDE
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
