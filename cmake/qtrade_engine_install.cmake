# ---------------------------------------------------------------------------
# Install rules for find_package(qtrade_engine CONFIG).
#
# Default prefix: /usr/local (override with -DCMAKE_INSTALL_PREFIX=).
# Installed layout:
#   lib/libqtrade_engine.a
#   include/qtrade/...（公开契约 + 实现头，路径仍为 qtrade/engine、qtrade/common）
#   config/qtrade_engine.json
#   lib/cmake/qtrade_engine/...
#
# 物理源已扁平到 src/qtrade_engine/{common,oms,...}；安装时映射回公开 include 路径，
# 使 #include "qtrade/engine/..." / "qtrade/common/..." 在安装树中仍成立。
# （长期应仅安装 include/ 下的对外头；client 依赖的实现头需先迁入 include/。）
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

# common → include/qtrade/common
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/qtrade_engine/common/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade/common
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
)

# Flattened engine modules → include/qtrade/engine/<module>
set(_qtrade_engine_install_modules
  account bridge cms core ems event_bus oms position risk strategy utils
)
foreach(_mod IN LISTS _qtrade_engine_install_modules)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/qtrade_engine/${_mod}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade/engine/${_mod}
    FILES_MATCHING
      PATTERN "*.hpp"
      PATTERN "*.h"
  )
endforeach()
unset(_qtrade_engine_install_modules)

# Top-level engine headers → include/qtrade/engine/
install(FILES
  ${CMAKE_SOURCE_DIR}/src/qtrade_engine/trading_engine.hpp
  ${CMAKE_SOURCE_DIR}/src/qtrade_engine/trading_engine_define.hpp
  ${CMAKE_SOURCE_DIR}/src/qtrade_engine/trading_engine_struct.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade/engine
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
