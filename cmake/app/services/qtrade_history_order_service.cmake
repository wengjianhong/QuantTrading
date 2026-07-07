# ---------------------------------------------------------------------------
# Standalone process: history order service (qtrade_history_order_service)
#
# Status: MVP stub (main only); split *_static lib when implementation lands
# Config: config/qtrade_history_order_service.json
# ---------------------------------------------------------------------------

# Build Service Executable
add_executable(qtrade_history_order_service ${QTRADE_APPS_DIR}/qtrade_history_order_service/main.cpp)
target_include_directories(qtrade_history_order_service PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_history_order_service PRIVATE qtrade_common)
