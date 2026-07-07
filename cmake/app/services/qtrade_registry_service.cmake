# ---------------------------------------------------------------------------
# Standalone process: service registry (qtrade_registry_service)
#
# Status: MVP stub (main only); etcd-based discovery in phase 2
# Config: config/qtrade_registry_service.json
# ---------------------------------------------------------------------------

# Build Service Executable
add_executable(qtrade_registry_service ${QTRADE_APPS_DIR}/qtrade_registry_service/main.cpp)
target_include_directories(qtrade_registry_service PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_registry_service PRIVATE qtrade_common)
