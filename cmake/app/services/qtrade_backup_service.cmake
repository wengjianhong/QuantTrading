# ---------------------------------------------------------------------------
# Standalone process: backup service (qtrade_backup_service)
#
# Status: MVP stub (main only); disaster recovery in phase 2
# Config: config/qtrade_backup_service.json
# ---------------------------------------------------------------------------

# Recursively collect implementation sources (includes repository/)
add_executable(qtrade_backup_service ${QTRADE_APPS_DIR}/qtrade_backup_service/main.cpp)
target_include_directories(qtrade_backup_service PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_backup_service PRIVATE qtrade_common)
