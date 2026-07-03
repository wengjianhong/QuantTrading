# ---------------------------------------------------------------------------
# Standalone process: log service (qtrade_log_service)
#
# Status: MVP stub (main only); split *_static lib when implementation lands
# Config: config/qtrade_log_service.json
# ---------------------------------------------------------------------------

# Build Service Executable
add_executable(qtrade_log_service ${QTRADE_APPS_DIR}/qtrade_log_service/main.cpp)
target_include_directories(qtrade_log_service PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_log_service PRIVATE qtrade_common)
