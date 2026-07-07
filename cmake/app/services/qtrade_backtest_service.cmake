# ---------------------------------------------------------------------------
# Standalone process: backtest service (qtrade_backtest_service)
#
# Status: MVP stub (main only); split *_static lib when implementation lands
# Config: config/qtrade_backtest_service.json
# ---------------------------------------------------------------------------

# Build Service Executable
add_executable(qtrade_backtest_service ${QTRADE_APPS_DIR}/qtrade_backtest_service/main.cpp)
target_include_directories(qtrade_backtest_service PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_backtest_service PRIVATE qtrade_common)
