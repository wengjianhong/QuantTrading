# ---------------------------------------------------------------------------
# Standalone process: trading engine (qtrade_engine)
# ---------------------------------------------------------------------------

# Build Engine Executable
add_executable(qtrade_engine ${QTRADE_APPS_DIR}/qtrade_engine/main.cpp)
target_include_directories(qtrade_engine PRIVATE
    ${QTRADE_SRC_DIR}
)
target_link_libraries(qtrade_engine PRIVATE qtrade_core)
