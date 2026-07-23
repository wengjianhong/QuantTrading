# ---------------------------------------------------------------------------
# Install rules for find_package(qtrade CONFIG).
#
# Default prefix: /usr/local (override with -DCMAKE_INSTALL_PREFIX=).
# Installed layout:
#   lib/libqtrade_{proto,common,core}.a
#   include/qtrade/...
#   include/qtrade_sdk/...
#   include/qtrade/proto/config/v1/*.pb.h (generated)
#   
#   lib/cmake/qtrade/qtrade-config.cmake
#   lib/cmake/qtrade/qtrade-config-version.cmake
#   lib/cmake/qtrade/qtradeTargets.cmake
#   bin/qtrade_engine, bin/qtrade_*_service
# ---------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/qtrade-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Libraries for downstream; EXPORT registers targets for install(EXPORT) below
install(TARGETS qtrade_proto qtrade_common qtrade_core
  EXPORT qtradeTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Public headers installed from include/qtrade/.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/qtrade/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/qtrade_sdk/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade_sdk
)

# Generated protobuf/gRPC headers (staged under qtrade/proto/)
if(QTRADE_PROTO_FILES)
  install(DIRECTORY ${QTRADE_PROTO_PUBLIC_INCLUDE_DIR}/qtrade/proto/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtrade/proto
  )
endif()

# Applications (not part of imported targets)
install(TARGETS
  qtrade_engine
  ${QTRADE_SERVICE_TARGETS}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Generate and install Targets file; NAMESPACE exposes qtrade::*
install(EXPORT qtradeTargets
  FILE qtradeTargets.cmake
  NAMESPACE qtrade::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qtrade
)

# Package config: includes qtradeTargets.cmake
install(FILES
  ${CMAKE_CURRENT_LIST_DIR}/qtrade-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/qtrade-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qtrade
)
