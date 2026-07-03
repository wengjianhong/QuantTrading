# ---------------------------------------------------------------------------
# Standalone process: audit service (qtrade_audit_service)
#
# Status: MVP stub (main only); split *_static lib when implementation lands
# Config: config/qtrade_audit_service.json
# ---------------------------------------------------------------------------

# Build Service Executable
add_executable(qtrade_audit_service ${QTRADE_APPS_DIR}/qtrade_audit_service/main.cpp)
target_include_directories(qtrade_audit_service PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_audit_service PRIVATE qtrade_common)
