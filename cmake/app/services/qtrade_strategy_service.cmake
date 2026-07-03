# ---------------------------------------------------------------------------
# Standalone process: strategy management service (qtrade_strategy_service)
#
# Status: MVP stub (main only); split *_static lib when implementation lands
# Config: config/qtrade_strategy_service.json
# ---------------------------------------------------------------------------

# Build Service Executable
add_executable(qtrade_strategy_service ${QTRADE_APPS_DIR}/qtrade_strategy_service/main.cpp)
target_include_directories(qtrade_strategy_service PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_strategy_service PRIVATE qtrade_common)
